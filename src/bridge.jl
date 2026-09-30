# Internal embedding adapter. C++ retains the returned objects with RAII roots;
# this file neither owns them nor maintains a registry of instances.

function _invoke_with_error_capture(f, args...)
    try
        return (Base.invokelatest(f, args...), nothing)
    catch exception
        return (nothing, sprint(showerror, exception, catch_backtrace()))
    end
end

function _resolve_filter_type(path::String)
    parts = split(path, '.'; keepempty=true)
    length(parts) >= 2 && all(Base.isidentifier, parts) ||
        throw(ArgumentError("invalid filter '$path': expected Package.filter"))
    namespace = Base.require(Main, Symbol(first(parts)))
    for component in parts[2:end-1]
        namespace = getproperty(namespace, Symbol(component))
        namespace isa Module || throw(ArgumentError("'$component' in '$path' is not a module"))
    end
    T = getproperty(namespace, Symbol(last(parts)))
    T isa Type || throw(ArgumentError("filter '$path' must name a type"))
    return T
end

function _create_filter(kind::UInt8, keys, values, expected)
    metadata = Dict(Symbol(String(k)) => String(v) for (k, v) in zip(keys, values))
    path = pop!(metadata, :filter, "")
    isempty(path) && throw(ArgumentError("Julia filter requires 'filter'"))
    pop!(metadata, :name, nothing) # COLA's factory selector, not a user parameter.
    T = _resolve_filter_type(path)
    T <: expected || error("filter '$path' has the wrong kind")
    # Package loading above may introduce new schema and constructor methods.
    object = Base.invokelatest(construct, T; metadata...)
    # Julia constructors can return an object of a different type.
    object isa expected || error("constructor '$path' returned the wrong kind")
    return object
end

_create_filter(kind::UInt8, keys, values) =
    _create_filter(kind, keys, values, (Generator, Converter, Writer)[Int(kind) + 1])

_create_unsafe_filter(kind::UInt8, keys, values) =
    _create_filter(kind, keys, values, (UnsafeGenerator, UnsafeConverter)[Int(kind) + 1])

function _event_result(event)
    event isa EventData || throw(ArgumentError("Julia filter must return EventData"))
    return event
end

_generate_event(filter::Generator) = _event_result(generate!(filter))
_convert_event(filter::Converter, event) = _event_result(convert!(filter, copy(event)))
_write_event(filter::Writer, event) = write!(filter, copy(event))

_process_unsafe_event(filter::UnsafeGenerator, event) = generate!(filter, event)
_process_unsafe_event(filter::UnsafeConverter, event) = convert!(filter, event)
