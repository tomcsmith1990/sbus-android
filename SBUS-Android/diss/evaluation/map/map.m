delimiter = '\n';

range1 = [0 0 99 0];
range2 = [100 0 199 0];

laptop_laptop = dlmread('spoke-map.txt', delimiter, range1);
laptop_phone = dlmread('spoke-map.txt', delimiter, range2);
phone_laptop = dlmread('spoke-map-phone.txt', delimiter, range1);
phone_phone = dlmread('spoke-map-phone.txt', delimiter, range2);

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
