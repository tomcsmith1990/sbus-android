delimiter = '\n';

range = [1 0 1000 0];

java = dlmread('jni-java.txt', delimiter, range);
cpp = dlmread('jni.txt', delimiter, range);

diff = java - cpp;

barh([0 1], [mean(cpp)/1000 mean(diff)/1000 ; 0 0 ], 0.2, 'stacked');
ylim([-.5 .5]);
set(gca, 'YTick', []);
xlabel('Breakdown of time taken to perform a JNI call (\mus)');
legend({'C++', 'Java'});

print -depsc 'jni.eps';
