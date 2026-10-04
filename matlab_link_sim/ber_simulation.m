%% BER_SIMULATION.M
% Monte Carlo BPSK/QPSK bit-error-rate simulation, verified against
% theoretical BER before trusting it for the link budget calculation.
%
% METHODOLOGY NOTE: uses ADAPTIVE bit counts (simulate more bits at
% higher Eb/N0) rather than a fixed count throughout. A fixed count
% (e.g. 200,000 bits) works fine at low Eb/N0 but is fundamentally
% unable to reliably measure low BER values -- at Eb/N0=10dB, theoretical
% BER~3.9e-6 means 200,000 bits only expects ~0.8 error events on
% average, so a single trial could show exactly zero errors purely by
% chance, which looks like "0 BER" but isn't a real measurement. This
% was caught during Python-side pre-verification (see docs) and fixed by
% requiring a minimum of 100 observed errors before accepting a BER
% estimate as statistically reliable, capped at a max bit count to keep
% runtime bounded.

clear; clc; close all;

%% Parameters
EbN0_dB_range = [0, 2, 4, 6, 8, 10, 12];
min_errors = 100;
chunk_size = 500000;
max_bits = 50e6;

n_points = length(EbN0_dB_range);
sim_BER = zeros(1, n_points);
theory_BER = zeros(1, n_points);
bits_used = zeros(1, n_points);
errors_found = zeros(1, n_points);

rng(42);  % reproducibility, matches the pre-verification run

%% BPSK Monte Carlo, adaptive bit count
for k = 1:n_points
    EbN0_dB = EbN0_dB_range(k);
    EbN0 = 10^(EbN0_dB/10);
    noise_std = sqrt(1/(2*EbN0));

    total_bits = 0;
    total_errors = 0;

    while total_errors < min_errors && total_bits < max_bits
        bits = randi([0 1], 1, chunk_size);
        symbols = 2*bits - 1;              % BPSK: 0->-1, 1->+1
        noise = noise_std * randn(1, chunk_size);
        rx = symbols + noise;
        bits_hat = double(rx > 0);
        total_errors = total_errors + sum(bits_hat ~= bits);
        total_bits = total_bits + chunk_size;
    end

    sim_BER(k) = total_errors / total_bits;
    theory_BER(k) = 0.5*erfc(sqrt(EbN0));
    bits_used(k) = total_bits;
    errors_found(k) = total_errors;

    fprintf('Eb/N0=%2ddB: sim=%.4e, theory=%.4e, ratio=%.3f, bits=%d, errors=%d\n', ...
        EbN0_dB, sim_BER(k), theory_BER(k), sim_BER(k)/theory_BER(k), ...
        total_bits, total_errors);

    if total_errors < min_errors
        fprintf('  -> NOTE: hit max_bits cap before reaching %d errors -- BER too\n', min_errors);
        fprintf('     low to reliably measure by direct Monte Carlo at this bit budget.\n');
        fprintf('     This is expected/correct behavior, not a simulation failure.\n');
    end
end

%% Plot: simulated vs theoretical BER curve
figure('Name', 'BPSK BER: Simulation vs Theory');
semilogy(EbN0_dB_range, theory_BER, 'b-', 'LineWidth', 1.5); hold on;
semilogy(EbN0_dB_range, sim_BER, 'ro', 'MarkerSize', 8, 'MarkerFaceColor', 'r');
xlabel('E_b/N_0 (dB)');
ylabel('Bit Error Rate');
title('BPSK BER: Monte Carlo Simulation vs Theoretical Q-function');
legend('Theory: 0.5*erfc(sqrt(EbN0))', 'Monte Carlo simulation', 'Location', 'southwest');
grid on;
ylim([1e-9, 1]);

saveas(gcf, '../results_figures/ber_bpsk_verification.png');
fprintf('\nFigure saved.\n');

%% QPSK Monte Carlo -- sanity check: per-bit BER should match BPSK theory
% exactly (QPSK is two independent BPSK streams on I/Q, so per-bit
% performance is identical to BPSK despite carrying 2 bits/symbol).
% This was verified in pre-check before writing this .m file:
% Eb/N0=0,4,8,10dB all matched BPSK theory within statistical variation
% (ratios 0.97-1.07 given ~100 observed errors), confirming the QPSK
% simulator itself is implemented correctly.

fprintf('\n--- QPSK verification (per-bit BER should match BPSK theory) ---\n');
qpsk_sim_BER = zeros(1, n_points);

for k = 1:n_points
    EbN0_dB = EbN0_dB_range(k);
    EbN0 = 10^(EbN0_dB/10);
    Es_N0 = 2*EbN0;      % QPSK: Es = 2*Eb (2 bits/symbol)
    N0 = 1/Es_N0;        % normalized Es=1

    total_bits = 0;
    total_errors = 0;

    while total_errors < min_errors && total_bits < max_bits
        n_sym = chunk_size/2;
        bits = randi([0 1], 1, n_sym*2);
        bI = bits(1:2:end); bQ = bits(2:2:end);
        sI = (2*bI-1)/sqrt(2); sQ = (2*bQ-1)/sqrt(2);
        noise_std = sqrt(N0/2);
        rI = sI + noise_std*randn(1,n_sym);
        rQ = sQ + noise_std*randn(1,n_sym);
        bI_hat = double(rI>0); bQ_hat = double(rQ>0);
        total_errors = total_errors + sum(bI_hat~=bI) + sum(bQ_hat~=bQ);
        total_bits = total_bits + n_sym*2;
    end

    qpsk_sim_BER(k) = total_errors/total_bits;
    fprintf('Eb/N0=%2ddB: QPSK sim=%.4e, BPSK theory=%.4e, ratio=%.3f\n', ...
        EbN0_dB, qpsk_sim_BER(k), theory_BER(k), qpsk_sim_BER(k)/theory_BER(k));
end

figure('Name', 'QPSK vs BPSK BER Comparison');
semilogy(EbN0_dB_range, theory_BER, 'b-', 'LineWidth', 1.5); hold on;
semilogy(EbN0_dB_range, sim_BER, 'ro', 'MarkerSize', 8, 'MarkerFaceColor', 'r');
semilogy(EbN0_dB_range, qpsk_sim_BER, 'gs', 'MarkerSize', 8, 'MarkerFaceColor', 'g');
xlabel('E_b/N_0 (dB)'); ylabel('Bit Error Rate');
title('BPSK vs QPSK: per-bit BER should coincide');
legend('BPSK theory', 'BPSK simulation', 'QPSK simulation', 'Location', 'southwest');
grid on; ylim([1e-9, 1]);
saveas(gcf, '../results_figures/ber_qpsk_comparison.png');
fprintf('\nQPSK comparison figure saved.\n');
