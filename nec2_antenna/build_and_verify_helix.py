"""
build_and_verify_helix.py

Builds the axial-mode helix antenna geometry in NEC2 format and verifies
it with necpp (open-source NEC2 engine) before handing the .nec file to
the user for their own 4NEC2 GUI. Same discipline as the ngspice work in
Stage 3: verify computationally first, don't hand over unverified files.
"""
import numpy as np
import necpp

f0 = 435e6
c = 299792458.0
lam = c / f0

C = lam
D = C / np.pi
r = D / 2
alpha_deg = 12.8
alpha = np.radians(alpha_deg)
S = C * np.tan(alpha)
n_turns = 10
segs_per_turn = 12
wire_radius_mm = 1.0  # ~2mm diameter wire, typical for a UHF helix element

# All dimensions in meters for NEC2 (NEC2 is unit-agnostic but must be
# internally consistent -- using meters throughout)
r_m = r
S_m = S
wire_rad_m = wire_radius_mm / 1000.0

nec = necpp.nec_create()

# Helix geometry: parametrize by angle theta from 0 to n_turns*2*pi,
# discretized into straight segments (segs_per_turn per full turn).
# Helix axis is z, base at z=some small offset above ground (z=0 is the
# ground plane) so the antenna sits just above it, standard for a
# ground-plane-fed axial helix.
z0 = 0.01  # 10mm feed gap above ground plane
n_total_segs = n_turns * segs_per_turn
theta = np.linspace(0, n_turns * 2 * np.pi, n_total_segs + 1)
x = r_m * np.cos(theta)
y = r_m * np.sin(theta)
z = z0 + (S_m / (2 * np.pi)) * theta

tag = 1
for i in range(n_total_segs):
    necpp.nec_wire(nec, tag, 1,
                    x[i], y[i], z[i],
                    x[i+1], y[i+1], z[i+1],
                    wire_rad_m, 1.0, 1.0)
    tag += 1

# Feed wire: short vertical segment from ground (z=0) up to the helix
# base (z0), directly below the first helix point -- this is the feed gap
necpp.nec_wire(nec, tag, 1, x[0], y[0], 0.0, x[0], y[0], z0, wire_rad_m, 1.0, 1.0)
feed_tag = tag

necpp.nec_geometry_complete(nec, 1)  # 1 = ground plane present

# Perfect ground plane at z=0
necpp.nec_gn_card(nec, 1, 0, 0, 0, 0, 0, 0, 0)

# Excitation: voltage source on the feed wire, segment 1 of that wire
necpp.nec_ex_card(nec, 0, feed_tag, 1, 0, 1.0, 0.0, 0, 0, 0, 0)

# Frequency: single point at design frequency (435MHz), in MHz for NEC2
necpp.nec_fr_card(nec, 0, 1, 435.0, 0.0)

# Request radiation pattern along the axial direction (theta=0, boresight)
necpp.nec_rp_card(nec, 0, 1, 1, 0, 5, 0, 0, 0.0, 0.0, 1.0, 1.0, 0.0, 0.0)

Zr = necpp.nec_impedance_real(nec, 0)
Zi = necpp.nec_impedance_imag(nec, 0)
gain_max = necpp.nec_gain_max(nec, 0)

print(f"=== NEC2 (necpp) verification results ===")
print(f"Feed impedance: {Zr:.2f} + j{Zi:.2f} ohm")
print(f"Kraus approximation was: ~140 + j0 ohm (resistive)")
print(f"Max gain: {gain_max:.2f} dBi")
print(f"Kraus approximation was: ~15.3 dBi")

necpp.nec_delete(nec)
