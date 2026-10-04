# Stage 5: MATLAB/Simulink Link-Level Verification — Findings

## BER Simulation (BPSK/QPSK)

**Methodology**: Monte Carlo simulation with ADAPTIVE bit counts, not a
fixed count. A fixed count (e.g. 200,000 bits) works at low Eb/N0 but
cannot reliably measure low BER -- at Eb/N0=10dB, theoretical BER~3.9e-6
means 200,000 bits only expects ~0.8 error events on average, so a
single trial could show exactly zero errors by chance, which looks like
"perfect" but isn't a real measurement. Fixed by requiring >=100 observed
errors before accepting a BER estimate, capped at 50M bits per point to
bound runtime.

**BPSK verification** (simulation vs. theoretical `0.5*erfc(sqrt(Eb/N0))`):

| Eb/N0 (dB) | Sim BER | Theory BER | Ratio | Bits used | Errors |
|---|---|---|---|---|---|
| 0 | 7.887e-2 | 7.865e-2 | 1.003 | 500,000 | 39,436 |
| 4 | 1.233e-2 | 1.250e-2 | 0.986 | 500,000 | 6,166 |
| 8 | 1.970e-4 | 1.909e-4 | 1.032 | 1,000,000 | 197 |
| 10 | 3.885e-6 | 3.872e-6 | 1.003 | 26,000,000 | 101 |
| 12 | 0 (cap hit) | 9.006e-9 | -- | 50,000,000 | 0 |

All measured points within ~3% of theory. The 12dB point hitting the bit
cap with zero errors is CORRECT, not a failure -- theory predicts <1
expected error in 50M bits at that Eb/N0, which is exactly why real
systems rely on the analytical formula at low-BER operating points
rather than pure simulation.

**QPSK verification**: per-bit BER matched BPSK theory within statistical
variation (ratios 0.97-1.07 at Eb/N0=0,4,8,10dB) -- confirms the QPSK
simulator is correctly implemented, since QPSK's two independent I/Q
BPSK streams should give identical per-bit performance to plain BPSK.

## Link Budget

**Inputs traceable to earlier stages, not assumed**:
- Matching network loss: 0dB (Stage 3's 53.87dB return loss = ~0.004%
  reflected, negligible)
- Frequency: 435MHz (same as Stage 3/4 design frequency)
- Antenna gain: 2dBi (realistic monopole/turnstile) -- DELIBERATELY not
  the Stage 4 helix's 10.5dBi, since a 1.57m helix doesn't fit a CubeSat
  per Stage 4's own finding; the link budget reflects what would
  actually fly, not the antenna-modeling demonstration

**Best-case vs. worst-case Eb/N0 across realistic LEO slant ranges**
(worst-case includes 3dB polarization mismatch + 1.5dB pointing loss +
1dB ground losses = 5.5dB total):

| Range (km) | Eb/N0 best (dB) | Eb/N0 worst (dB) |
|---|---|---|
| 500 | 47.84 | 42.34 |
| 1200 | 40.23 | 34.73 |
| 2000 | 35.80 | 30.30 |
| 2500 | 33.86 | 28.36 |

**KEY FINDING: this design is massively over-margined at the conservative
1200bps baseline rate.** Max range for a worst-case 10dB Eb/N0 target is
~20,698km -- 8x beyond any realistic LEO slant range (~2500km max at low
elevation). This isn't a bad result -- it reveals real design headroom.

**Data rate trade study** (at worst-case 2500km range, all extra losses):

| Rb (bps) | Eb/N0 (dB) |
|---|---|
| 1200 | 28.36 |
| 9600 | 19.33 |
| 38400 | 13.31 |
| 76800 | 10.30 |

**Conclusion**: this beacon design could support up to ~76.8kbps (64x the
conservative baseline) while still closing with ~10dB margin at
worst-case range. This is a genuine systems-engineering insight this
project's earlier stages made possible -- the negligible matching-network
loss (Stage 3) and honest antenna gain assumption (informed by Stage 4's
size finding) both feed directly into a link budget that answers a real
design question, not just "does 1200bps work" (trivially yes).

## Ground Station Pointing

**Verification**: the ECEF->ENU coordinate transform was checked against
a known degenerate case (satellite placed directly overhead a ground
station) before trusting it for the full pass simulation -- this MUST
give exactly elevation=90deg and range=orbital altitude with zero free
parameters to tune. Result: el=90.0000deg, range=500.0km for a 500km-
altitude test case -- exact match, confirming the transform is correct.

**Simplification, documented**: uses a simplified circular-orbit
propagator (same category as the completed CubeSat-ADCS-Sim project's
orbit_propagator.m), not full SGP4/TLE propagation -- adequate for
demonstrating the pointing-angle computation methodology, not claimed as
mission-operations-grade orbit determination.

## Files
- `ber_simulation.m` -- BPSK/QPSK Monte Carlo verification
- `link_budget.m` -- full link budget + data rate trade study
- `ground_pointing.m` -- az/el ground station pointing, with built-in
  self-verification against the overhead-satellite degenerate case
