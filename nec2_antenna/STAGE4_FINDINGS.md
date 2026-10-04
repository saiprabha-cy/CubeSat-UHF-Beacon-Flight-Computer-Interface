# Stage 4: Helical Antenna Model — Findings & Verification

## Design (Kraus axial-mode helix, f=435MHz)

```
Wavelength:      689.18 mm
Circumference:   689.18 mm (= 1 lambda, optimal for axial mode)
Diameter:        219.37 mm
Pitch angle:     12.8 deg (near-optimal per Kraus)
Spacing/turn:    156.58 mm
Turns:           10
Total length:    1565.77 mm (1.57 m)
Wire diameter:   2 mm
Feed:            ground-plane fed, perfect infinite ground assumed
```

## SIZE REALITY CHECK — a real, important finding

**1.57m axial length is ~4.6x longer than a 3U CubeSat's 340mm long axis.**
A full free-space axial-mode helix at 435MHz simply does not fit a
CubeSat body. This is exactly why real CubeSats almost universally use
deployable monopole, dipole, or turnstile wire antennas at UHF instead of
helices — this is not a design mistake here, it's the honest outcome of
following the project bank's assigned antenna type (helical, for
methodology demonstration) through to its real physical conclusion. A
real mission choosing a helix for higher gain would need it as a
separate deployable structure, not body-mounted — worth stating plainly
rather than glossing over.

## Verification methodology

**Two independent tools, cross-checked exactly**: `necpp` (Python-bound
open-source NEC2 engine) for iterative development, and `nec2c` (a
separate standalone CLI implementation) reading the final generated
`.nec` card file directly. Both report **identical** results
(`Zin = 200.47 - j45.01 ohm`, `Gain = 10.50 dB` at the same segmentation)
— this confirms the geometry logic is correct AND the raw `.nec` file
syntax is valid, not just that my Python construction of it was
internally consistent.

## Convergence study (segmentation resolution)

| segs/turn | total segs | Zr (ohm) | Zi (ohm) | Gain (dBi) |
|---|---|---|---|---|
| 8 | 80 | 185.59 | -44.71 | 10.38 |
| 12 | 120 | 191.41 | -44.38 | 10.55 |
| 16 | 160 | 195.41 | -44.48 | 10.58 |
| 24 | 240 | 200.47 | -45.01 | 10.56 |
| 32 | 320 | 203.46 | -45.46 | 10.53 |
| 40 | 400 | 205.41 | -45.80 | 10.50 |
| 48 | 480 | 206.78 | -46.06 | 10.48 |
| 64 | 640 | 208.65 | -46.46 | 10.45 |
| 80 | 800 | 209.98 | -46.78 | 10.43 |
| 100 | 1000 | 211.33 | -47.14 | 10.40 |
| 150 | 1500 | 214.29 | -48.03 | 10.34 |

**Gain**: converged to 10.3-10.6 dBi (tight, ~0.3dB spread across a
20x segmentation range) -- trustworthy.

**Reactance**: converged to -44 to -48 ohm -- trustworthy.

**Resistance**: shows slow, decelerating drift (186 -> 214 ohm) even at
1500 segments. This is a known, documented NEC2 characteristic: feed-point
resistance for wire antennas converges slowly near the local
source-segment field singularity under uniform method-of-moments
segmentation -- not a bug in this model. Reported honestly as
**approximately 190-215 ohm**, rather than a single falsely-precise
number. The real engineering conclusion this drives -- a significant
impedance mismatch from the 50ohm system, requiring dedicated matching if
this antenna type were actually used -- does not change regardless of
which value within that range is exact.

## Design implication

Comparing to the Kraus closed-form approximation (R~140ohm, Gain~15.3dBi):
the NEC2 numerical result diverges meaningfully from the simple formula
(R is ~40-55% higher, gain is ~5dB lower). This is expected and
well-documented -- Kraus's formulas are empirical curve fits from 1940s-50s
measurements under specific ground-plane conditions, while NEC2 solves
Maxwell's equations numerically via method of moments for THIS exact
geometry and ground assumption (infinite perfect ground here, which
differs from Kraus's finite test ground planes). The NEC2/method-of-moments
result is the one to trust for this specific model.

**Practical note**: a real implementation of this antenna (if the size
were solved via a deployable structure) would need its own impedance
transformer (~200ohm to 50ohm) at the feed, separate from the earlier
Stage 3 matching network -- which was designed for a 50ohm antenna
assumption. This is a legitimate open item a real mission would need to
resolve, consistent with the project's stated realistic engineering
scope over an idealized one.

## Files
- `build_and_verify_helix.py` -- Python/necpp geometry generation + convergence study
- `helix_antenna.nec` -- final portable NEC2 card file (240 segs/turn=24),
  verified identically by both necpp and nec2c
- `helix_antenna.out` -- nec2c CLI run output (full verification record)

## For your own run
Open `helix_antenna.nec` directly in 4NEC2 (File -> Open). The frequency
(435MHz), ground plane, and excitation are already set via the FR/GN/EX
cards in the file -- just run the analysis and compare against the
Zin/gain values above.
