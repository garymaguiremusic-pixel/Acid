# Design decisions

Choices made where the spec left room. In each case the aim was an authentic,
playable acid sound.

## Build and format
- **Windows**: VST3. **macOS**: VST3 + AU, universal (arm64 + x86_64), macOS 10.13+,
  shipped as an installer .pkg (the same approach as OTT): the plugins are ad-hoc
  signed, and because Installer puts them in place they carry no quarantine flag, so
  DAWs load them without the malware warning. The .pkg itself is unsigned (right-click >
  Open once). `packaging/macos/build_installer.sh` can also Developer-ID sign and
  notarize if signing identities and Apple credentials are ever provided. An optional
  Standalone target (`-DACID_BUILD_STANDALONE=ON`) is there for local testing.
- **JUCE 8.0.4**, the same version Virus Delay builds with, fetched by CMake.
- The plugin name lives in one CMake cache variable, `ACID_PLUGIN_NAME` (default
  "Acid"). It flows into the product name, the preset folder and the UI.

## GUI
- **Look and feel**: taken from Virus Delay at its latest restyle (black panels, orange
  as the only accent, light grey labels, white highlights, chrome large knobs, black
  small knobs, LCD readouts, mode-style page tabs, bottom title bar with a "?" help).
  All drawing is procedural. There are no image assets and no third-party branding.
- **Size**: the base layout is 1000 x 900 (not the 1600 x 1176 in the spec), so the
  Virus Delay font sizes and control sizes read the same. The window resizes from
  60 % to 200 % with a fixed aspect ratio, scales the whole panel, and remembers its
  size.
- **Knobs** move in whole steps of 1.0: dragging, and the mouse wheel (one notch = one
  step). Double-click resets to the default. Where a real unit fits whole steps
  (semitones, dB, %, ms, steps, voices, degrees) the parameter uses that unit
  directly. Otherwise it runs 0..100 and is mapped to real units (Hz, ms), which the
  knob, the help display and the host all show.
- **Hover help** goes in the header display (bank / preset name / help line) instead
  of tooltips. Controls that came from ArpSampler show their tooltip text there too.
- **Light theme**: a pale-grey variant with the same orange accent. The theme is saved
  with the session.
- The **Steps** page buttons (1-16 ... 49-64) sit in the sequencer's control row. The
  pattern **Length** is part of the pattern data (each pattern has its own length,
  with undo), so the Length knob is not an automatable parameter. Clicking a step
  number in the grid also sets the length.

## Sound
- **Filter**: a ZDF/TPT ladder solved exactly every sample (4x4 tridiagonal system)
  with the feedback loop resolved instantly and the ladder input saturated. The diode
  model uses coupled stages with a 110 Hz high-pass in the feedback path, and its
  feedback stops at 94 % of the self-oscillation point (squelch without a clean
  whistle). The transistor model can self-oscillate. Cutoff is calibrated so the
  resonant peak sits on the knob's frequency. Both run at 2x oversampling, together
  with the oscillators and the drive.
- **Cutoff Low** is the floor the envelope falls back to. When it is at or above
  Cutoff, the filter behaves exactly like the classic circuit (base = Cutoff, the
  envelope adds Env Mod on top).
- **Accent** has its own envelope (Env Attack / Acc Decay) that drives the VCA boost and
  the accent circuit. Acc Sweep sets how slowly the accent "capacitor" charges and
  discharges, so runs of accents build up (covered by a unit test). Accented notes also
  use Acc Decay for the filter envelope.
- **Amp envelope**: Decay at its maximum means *hold*, and that is the default. Slid and
  tied notes then sustain like the hardware; the 170 ms default from the reference
  screenshot cut tied notes short. Attack (0.25 ms) and Release (4.5 ms) use the
  screenshot defaults.
- **LFO Mirror** makes the LFO unipolar (0..1) instead of bipolar. The Depth sign
  already covers inverting.
- **LFO depth ranges**: Cutoff +-4 octaves, Pitch +-12 semitones, Wave +-full morph,
  Reso +-full range, Volume = tremolo down to silence at 100 %.
- **Tube**: each band is divided by the square root of its tube's small-signal gain,
  so zero drive is unity and more drive adds mostly saturation with a moderate rise
  in level. The tube curve is the spec's biased tanh,
  normalised so +-1 in gives about +-1 out. Bypassing keeps the oversampling latency
  (the dry path is delayed to match), so the host's delay compensation never changes.
- **Output**: a gentle safety soft-clip above about -2 dBFS on the master. Factory
  presets peak around -7 to -5 dBFS.
- **Latency** reported to the host: 15 samples (voice oversampling) + 19 samples (tube
  oversampling).

## Sequencer and MIDI
- The sequencer is **ArpSampler's arpeggiator and step sequencer**, ported with its code
  (arp engine, pattern model, scales, randomiser, MIDI writer, import, export, sequence
  browser). Acid's **Glide** (the 303 slide time) is kept, in the Step Sequencer panel. A
  Tie to a new pitch glides over it.
- The synth and the sequencer share one page ("Synth / Seq"), so the window base size is
  1000 x 900.
- Acid is monophonic: Start events retrigger the voice, Change events (ties) glide without
  retriggering, and a Stop only ends the note that still owns the voice. In Chord mode the
  newest (top) note plays.
- **Accent** drives Acid's accent circuit. Loudness = velocity x the step's Volume, as in ArpSampler.
- **Base octave**: transpose 0 plays the Key's root at C1 (MIDI 36), not ArpSampler's C3,
  because Acid is a bass synth. Auto Play, Drag MIDI's Auto root and MIDI / audio import all
  use it.
- **Defaults**: Arp On, Step mode, Auto Play on (the pattern plays with the host, like the
  first Acid sequencer), Gate 50 %, MIDI Out off (as in ArpSampler).
- **Left out**: the Reverse lane (no sample to play backwards), Retrigger (always on; ties
  give the legato), and the sampler-only Key Lock / Quantise switches.
- **Older projects and presets** from the first Acid sequencer load their first pattern,
  converted: slide becomes Tie, a tie step holds the previous note, rest becomes Volume 0,
  ratchets carry over.
- Presets keep the randomiser and export settings, the internal tempo, MIDI Out and Auto
  Play, as ArpSampler's do. Loading a preset is one undo step for the pattern.
- **Factory presets** are built into the plugin. User presets are JSON files in
  `Documents/Acid/Presets` (params + the pattern + name + bank tag). Hosts that
  browse "programs" see the factory presets.
- File layout: the spec named `DiodeLadder` / `TransistorLadder` / `AccentCircuit` /
  `Unison` separately. They live in `dsp/Ladder` (one class, two models),
  `dsp/Envelopes` (envelopes + accent circuit) and `dsp/Voice` (unison is part of the
  voice). The sequencer, including its undo, is ArpSampler's `arp/PatternModel`.
