# vivid-sequencers

`vivid-sequencers` is a Vivid package library that provides sequencing control operators:

- `sequencer`
- `drum_sequencer`
- `pattern_seq`
- `note_pattern`
- `note_duration`
- `arpeggiator`
- `chord_progression`
- `state_machine`

## Contents

- `src/*.cpp` for the 8 operators
- `graphs/arpeggiator_demo.json`
- `graphs/chord_progression_demo.json`
- `graphs/control_demo.json`
- `graphs/state_machine_demo.json`
- `graphs/pattern_algebra_demo.json`
- `tests/test_package_manifest.cpp`
- `tests/test_state_machine.cpp`
- `tests/test_pattern_algebra.cpp`
- `vivid-package.json`

## Local development

From vivid-core:

```bash
./build/vivid link ../vivid-sequencers
./build/vivid rebuild vivid-sequencers
```

## CI smoke coverage

The package CI workflow:

1. Clones and builds vivid-core (`test_demo_graphs` + core operators).
2. Builds package operators and package tests.
3. Runs package tests.
4. Runs graph smoke tests against this package's `graphs/` directory.

## License

MIT (see `LICENSE`).
