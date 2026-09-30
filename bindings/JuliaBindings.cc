#define define_julia_module define_generated
#include "jlCOLANative.cxx"  // NOLINT(bugprone-suspicious-include): shared generated type traits
#undef define_julia_module

// Generated aggregate getters copy data. These accessors return native references.
// Keep generated type traits and custom accessors in one translation unit.

// NOLINTNEXTLINE(readability-identifier-naming): CxxWrap entry point
JLCXX_MODULE define_cola_julia(jlcxx::Module& module) {
  define_generated(module);
  using cola::EventData;
  using cola::EventIniState;
  using cola::LorentzVector;
  using cola::Particle;
  module.method("copy_event", [](const EventData& event) { return EventData(event); });
  module.method("initial_state", [](EventData& event) -> EventIniState& { return event.ini_state; });
  module.method("particles", [](EventData& event) -> cola::EventParticles& { return event.particles; });
  module.method("initial_state_particles",
                [](EventIniState& state) -> cola::EventParticles& { return state.ini_state_particles; });
  module.method("position", [](Particle& particle) -> LorentzVector& { return particle.position; });
  module.method("momentum", [](Particle& particle) -> LorentzVector& { return particle.momentum; });
  // WrapIt does not expose the anonymous union in LorentzVector.
  module.method("energy", [](const LorentzVector& vector) { return vector.e; });
  module.method("energy!", [](LorentzVector& vector, double x) { vector.e = x; });
  module.method("time", [](const LorentzVector& vector) { return vector.t; });
  module.method("time!", [](LorentzVector& vector, double x) { vector.t = x; });
  module.method("x", [](const LorentzVector& vector) { return vector.x; });
  module.method("x!", [](LorentzVector& vector, double x) { vector.x = x; });
  module.method("y", [](const LorentzVector& vector) { return vector.y; });
  module.method("y!", [](LorentzVector& vector, double x) { vector.y = x; });
  module.method("z", [](const LorentzVector& vector) { return vector.z; });
  module.method("z!", [](LorentzVector& vector, double x) { vector.z = x; });
}
