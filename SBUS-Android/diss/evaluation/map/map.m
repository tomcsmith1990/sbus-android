file = 'construct-lookup';

delimiter = '\n';

rangeLaptop = [0 0 99 0];
rangePhone = [100 0 199 0];

laptop = dlmread('spoke-map.txt', delimiter, rangeLaptop);
phone = dlmread('spoke-map.txt', delimiter, rangePhone);

disp('Laptop mean = ')
disp(mean(laptop))

disp('Laptop std = ')
disp(std(laptop))

disp('Phone mean = ')
disp(mean(phone))

disp('Phone std = ')
disp(std(phone))
