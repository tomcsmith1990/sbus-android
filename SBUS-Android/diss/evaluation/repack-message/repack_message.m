file = 'repack-message';

delimiter = '\n';

range = [1 0 100 0];

repack = zeros(4,4);

repack(1,1) = mean(dlmread(strcat(file, '-TinyConsumer-TinySensor.txt'), delimiter, range));
repack(1,2) = mean(dlmread(strcat(file, '-TinyConsumer-SmallSensor.txt'), delimiter, range));
repack(1,3) = mean(dlmread(strcat(file, '-TinyConsumer-BigSensor.txt'), delimiter, range));
repack(1,4) = mean(dlmread(strcat(file, '-TinyConsumer-GiantSensor.txt'), delimiter, range));
repack(2,1) = mean(dlmread(strcat(file, '-SmallConsumer-TinySensor.txt'), delimiter, range));
repack(2,2) = mean(dlmread(strcat(file, '-SmallConsumer-SmallSensor.txt'), delimiter, range));
repack(2,3) = mean(dlmread(strcat(file, '-SmallConsumer-BigSensor.txt'), delimiter, range));
repack(2,4) = mean(dlmread(strcat(file, '-SmallConsumer-GiantSensor.txt'), delimiter, range));
repack(3,1) = mean(dlmread(strcat(file, '-BigConsumer-TinySensor.txt'), delimiter, range));
repack(3,2) = mean(dlmread(strcat(file, '-BigConsumer-SmallSensor.txt'), delimiter, range));
repack(3,3) = mean(dlmread(strcat(file, '-BigConsumer-BigSensor.txt'), delimiter, range));
repack(3,4) = mean(dlmread(strcat(file, '-BigConsumer-GiantSensor.txt'), delimiter, range));
repack(4,1) = mean(dlmread(strcat(file, '-GiantConsumer-TinySensor.txt'), delimiter, range));
repack(4,2) = mean(dlmread(strcat(file, '-GiantConsumer-SmallSensor.txt'), delimiter, range));
repack(4,3) = mean(dlmread(strcat(file, '-GiantConsumer-BigSensor.txt'), delimiter, range));
repack(4,4) = mean(dlmread(strcat(file, '-GiantConsumer-GiantSensor.txt'), delimiter, range));

colormap('jet');
imagesc(repack);
c = colorbar;
ylabel(c, 'Time (\mus)');
title('Time taken to repackage a message for various sensor and consumer sizes');
ylabel('Consumer');
xlabel('Sensor');

sizes = ['Tiny ' ; 'Small' ; 'Big  ' ; 'Giant'];
set(gca, 'XTickLabel', sizes);
set(gca, 'YTickLabel', sizes);
set(gca, 'YTick', [1 2 3 4]);
set(gca, 'XTick', [1 2 3 4]);

print -depsc 'repack_message.eps'