file = 'construct-lookup';

delimiter = '\n';

range = [1 0 100 0];

lookup = zeros(4,4);

lookup(1,1) = mean(dlmread(strcat(file, '-TinyConsumer-TinySensor.txt'), delimiter, range));
lookup(1,2) = mean(dlmread(strcat(file, '-TinyConsumer-SmallSensor.txt'), delimiter, range));
lookup(1,3) = mean(dlmread(strcat(file, '-TinyConsumer-BigSensor.txt'), delimiter, range));
lookup(1,4) = mean(dlmread(strcat(file, '-TinyConsumer-GiantSensor.txt'), delimiter, range));
lookup(2,1) = mean(dlmread(strcat(file, '-SmallConsumer-TinySensor.txt'), delimiter, range));
lookup(2,2) = mean(dlmread(strcat(file, '-SmallConsumer-SmallSensor.txt'), delimiter, range));
lookup(2,3) = mean(dlmread(strcat(file, '-SmallConsumer-BigSensor.txt'), delimiter, range));
lookup(2,4) = mean(dlmread(strcat(file, '-SmallConsumer-GiantSensor.txt'), delimiter, range));
lookup(3,1) = mean(dlmread(strcat(file, '-BigConsumer-TinySensor.txt'), delimiter, range));
lookup(3,2) = mean(dlmread(strcat(file, '-BigConsumer-SmallSensor.txt'), delimiter, range));
lookup(3,3) = mean(dlmread(strcat(file, '-BigConsumer-BigSensor.txt'), delimiter, range));
lookup(3,4) = mean(dlmread(strcat(file, '-BigConsumer-GiantSensor.txt'), delimiter, range));
lookup(4,1) = mean(dlmread(strcat(file, '-GiantConsumer-TinySensor.txt'), delimiter, range));
lookup(4,2) = mean(dlmread(strcat(file, '-GiantConsumer-SmallSensor.txt'), delimiter, range));
lookup(4,3) = mean(dlmread(strcat(file, '-GiantConsumer-BigSensor.txt'), delimiter, range));
lookup(4,4) = mean(dlmread(strcat(file, '-GiantConsumer-GiantSensor.txt'), delimiter, range));

colormap(winter(ceil(max(max(lookup)))));
image(lookup);
c = colorbar;
ylabel(c, 'Time (\mus)', 'FontSize', 12);
title('Time taken to construct a lookup table for various producer and consumer sizes', 'FontSize', 12);
ylabel('Consumer', 'FontSize', 12);
xlabel('Producer', 'FontSize', 12);

sizes = ['Tiny ' ; 'Small' ; 'Big  ' ; 'Giant'];
set(gca, 'XTickLabel', sizes);
set(gca, 'YTickLabel', sizes);
set(gca, 'YTick', [1 2 3 4]);
set(gca, 'XTick', [1 2 3 4]);

[x,y] = meshgrid(1:4, 1:4);
text(x(:),y(:),num2str(lookup(:), '%.2f'), 'HorizontalAlignment', 'center', 'FontSize', 12);

print -depsc 'construct_lookup.eps'
