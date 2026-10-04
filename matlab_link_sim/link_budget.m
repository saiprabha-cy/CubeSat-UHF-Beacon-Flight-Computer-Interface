%% LINK_BUDGET.M
% Full link budget for the CubeSat UHF beacon, connecting real verified
% values from every prior stage into one system-level answer: does this
% beacon actually close a link to a ground station, and with how much
% margin.
%
% INPUTS TRACEABLE TO EARLIER STAGES (not assumed):
%   - Matching network insertion loss: Stage 3, 53.87dB return loss
%     measured -> ~0.004% power reflected -> effectively 0dB loss
%   - Frequency: 435MHz, same design frequency as Stages 3 and 4
%   - Antenna gain: deliberately NOT the Stage 4 helix's 10.5dBi -- a
%     1.57m helix does not fit a CubeSat (Stage 4's own finding), so a
%     realistic monopole/turnstile gain (~2dBi) is used instead here,
%     since the link budget should reflect what would actually fly.

clear; clc; close all;

%% Constants
c = 299792458;          % m/s
f = 435e6;               % Hz, matches Stage 3/4 design frequency
lambda = c/f;
k_boltz = 1.380649e-23;  % J/K

%% Link parameters (each one justified, not just assumed)
P_tx_dBW = 0.0;          % 1W -- placeholder pending Stage 2 (U1) final selection
G_tx_dBi = 2.0;          % realistic monopole/turnstile (NOT the 10.5dBi helix -- see note above)
L_matching_dB = 0.0;     % Stage 3 verified: 53.87dB return loss ~ negligible insertion loss
G_rx_dBi = 12.0;         % typical ground-station UHF yagi
T_sys_K = 300;           % typical unamplified UHF ground receiver system noise temp
Rb = 1200;               % bps, conservative CubeSat beacon data rate (baseline case)

% Worst-case additional losses (realistic degradations, not best-case only)
L_polarization_dB = 3.0; % linear monopole vs typically circularly-polarized ground antenna
L_pointing_dB = 1.5;     % imperfect ground-station tracking
L_ground_dB = 1.0;       % cable/connector losses
L_extra_dB = L_polarization_dB + L_pointing_dB + L_ground_dB;

EIRP_dBW = P_tx_dBW + G_tx_dBi - L_matching_dB;
N0_dBW_Hz = 10*log10(k_boltz*T_sys_K);

fprintf('=== Link Budget: CubeSat UHF Beacon @ 435MHz ===\n');
fprintf('EIRP = %.2f dBW\n', EIRP_dBW);
fprintf('Worst-case extra losses = %.1f dB (polarization+pointing+ground)\n\n', L_extra_dB);

%% Best-case vs worst-case Eb/N0 across realistic LEO slant ranges
d_range_km = [500, 800, 1200, 1600, 2000, 2500];
n = length(d_range_km);
EbN0_best = zeros(1,n);
EbN0_worst = zeros(1,n);

fprintf('%8s %16s %17s\n', 'd(km)', 'Eb/N0 best(dB)', 'Eb/N0 worst(dB)');
for i = 1:n
    d_m = d_range_km(i)*1000;
    Lp_dB = 20*log10(4*pi*d_m/lambda);
    Prx_best = EIRP_dBW - Lp_dB + G_rx_dBi;
    Prx_worst = Prx_best - L_extra_dB;
    EbN0_best(i) = Prx_best - N0_dBW_Hz - 10*log10(Rb);
    EbN0_worst(i) = Prx_worst - N0_dBW_Hz - 10*log10(Rb);
    fprintf('%8d %16.2f %17.2f\n', d_range_km(i), EbN0_best(i), EbN0_worst(i));
end

%% KEY FINDING: this design is massively over-margined at 1200bps.
% Compute max range for a 10dB worst-case Eb/N0 target (a standard "good
% margin" design point) to show just how much margin exists.
target_EbN0 = 10.0;
Prx_needed = target_EbN0 + N0_dBW_Hz + 10*log10(Rb);
Lp_max_dB = EIRP_dBW - L_extra_dB + G_rx_dBi - Prx_needed;
d_max_km = (10^(Lp_max_dB/20)) * lambda / (4*pi) / 1000;
fprintf('\nMax range for worst-case Eb/N0=%.0fdB @ Rb=%dbps: %.0f km\n', ...
    target_EbN0, Rb, d_max_km);
fprintf('(Realistic LEO worst-case slant range is ~2500km -- this design\n');
fprintf(' closes with enormous margin at 1200bps, %.1fx more range than needed)\n', ...
    d_max_km/2500);

%% Trade study: what data rate could this design actually support?
% At worst-case range (2500km, all extra losses), sweep Rb to find how
% much higher a data rate still closes with reasonable margin.
d_worst_km = 2500;
Lp_worst = 20*log10(4*pi*d_worst_km*1000/lambda);
Prx_worst_fixed = EIRP_dBW - Lp_worst + G_rx_dBi - L_extra_dB;

Rb_options = [1200, 4800, 9600, 19200, 38400, 76800];
fprintf('\n=== Data rate trade study (worst-case 2500km range) ===\n');
fprintf('%10s %12s\n', 'Rb(bps)', 'Eb/N0(dB)');
for Rb_test = Rb_options
    EbN0_test = Prx_worst_fixed - N0_dBW_Hz - 10*log10(Rb_test);
    fprintf('%10d %12.2f\n', Rb_test, EbN0_test);
end
fprintf('\nCONCLUSION: this beacon design could support up to ~76.8kbps (64x\n');
fprintf('the conservative 1200bps baseline) while still closing with ~10dB\n');
fprintf('margin at worst-case range -- real design headroom, not just a\n');
fprintf('pass/fail answer at one assumed data rate.\n');

%% Plot
figure('Name', 'Link Budget: Eb/N0 vs Range');
plot(d_range_km, EbN0_best, 'b-o', 'LineWidth', 1.5); hold on;
plot(d_range_km, EbN0_worst, 'r-s', 'LineWidth', 1.5);
yline(10, '--k', '10dB reference (good margin)');
xlabel('Slant range (km)'); ylabel('E_b/N_0 (dB)');
title('Link Budget: Eb/N0 vs Range (Rb=1200bps)');
legend('Best case', 'Worst case (+5.5dB losses)', 'Location', 'best');
grid on;
saveas(gcf, '../results_figures/link_budget_vs_range.png');

figure('Name', 'Data Rate Trade Study');
EbN0_vs_Rb = arrayfun(@(Rb) Prx_worst_fixed - N0_dBW_Hz - 10*log10(Rb), Rb_options);
semilogx(Rb_options, EbN0_vs_Rb, 'g-d', 'LineWidth', 1.5, 'MarkerFaceColor', 'g');
hold on; yline(10, '--k', '10dB reference');
xlabel('Data rate (bps)'); ylabel('E_b/N_0 (dB) at worst-case 2500km');
title('Data Rate Trade Study: how much rate can this design support?');
grid on;
saveas(gcf, '../results_figures/data_rate_trade_study.png');

fprintf('\nFigures saved.\n');
