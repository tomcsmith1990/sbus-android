delimiter = '\n';

range = [0 0 99 0];

laptop = dlmread('spoke-map-laptop.txt', delimiter, range) / 1000;
phone = dlmread('spoke-map-phone.txt', delimiter, range) / 1000;
remote = dlmread('spoke-map-remote.txt', delimiter, range) / 1000;

disp('laptop mean = ')
disp(mean(laptop))

disp('laptop std = ')
disp(std(laptop))

disp('phone mean = ')
disp(mean(phone))

disp('phone std = ')
disp(std(phone))

disp('remote mean = ')
disp(mean(remote))

disp('remote std = ')
disp(std(remote))

hold all;

errorbar(1, mean(remote), std(remote), '-o');
errorbar(2, mean(laptop), std(laptop), '-+');
errorbar(3, mean(phone), std(phone), '-*');

xlabel('Producer', 'FontSize', 12);
ylabel('Time (ms)', 'FontSize', 12);
title('Connection Times Between Producers on Different Devices', 'FontSize', 12);
set(gca, 'XTick', [1 2 3]);
set(gca, 'XTickLabel', {'Local Machine', 'Remote Machine', 'Remote Phone'});

print -depsc 'map.eps';

close all;

