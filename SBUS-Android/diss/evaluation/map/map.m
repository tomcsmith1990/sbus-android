delimiter = '\n';

range1 = [0 0 99 0];
range2 = [100 0 199 0];

laptop_laptop = dlmread('spoke-map.txt', delimiter, range1) / 1000;
laptop_phone = dlmread('spoke-map.txt', delimiter, range2) / 1000;
phone_laptop = dlmread('spoke-map-phone.txt', delimiter, range1) / 1000;
phone_phone = dlmread('spoke-map-phone.txt', delimiter, range2) / 1000;

disp('laptop_laptop mean = ')
disp(mean(laptop_laptop))

disp('laptop_laptop std = ')
disp(std(laptop_laptop))

disp('laptop_phone mean = ')
disp(mean(laptop_phone))

disp('laptop_phone std = ')
disp(std(laptop_phone))

disp('phone_laptop mean = ')
disp(mean(phone_laptop))

disp('phone_laptop std = ')
disp(std(phone_laptop))

disp('phone_phone mean = ')
disp(mean(phone_phone))

disp('phone_phone std = ')
disp(std(phone_phone))

hold all;

errorbar(1, mean(laptop_laptop), std(laptop_laptop), '-o');
errorbar(2, mean(laptop_phone), std(laptop_phone), '-+');
errorbar(3, mean(phone_laptop), std(phone_laptop), '-*');
errorbar(4, mean(phone_phone), std(phone_phone), '-X');

xlabel('Consumer-Producer', 'FontSize', 12);
ylabel('Time (ms)', 'FontSize', 12);
title('Connection Times Between Producers and Consumers on a Laptop and Phone', 'FontSize', 12);
set(gca, 'XTick', [1 2 3 4]);
set(gca, 'XTickLabel', {'Laptop-Laptop', 'Laptop-Phone', 'Phone-Laptop', 'Phone-Phone'});

print -depsc 'map.eps';

close all;

