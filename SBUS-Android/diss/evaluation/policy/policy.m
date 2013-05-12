data = dlmread('airs.txt');

policies = [1 10 100 1000];
times = [mean(data(:,1)) mean(data(:, 2)) mean(data(:, 3)) mean(data(:, 4))];

hold all;

plot(policies, times/1000000);

title('Time between receiving an event from AIRS-SBUS gateway and checking all policies', 'FontSize', 12);
xlabel('Number of Policies', 'FontSize', 12);
ylabel('Time to check all policies (milliseconds)', 'FontSize', 12);

print -depsc 'policy.eps';

close all;
