no_file = 'rdc-results.txt';
sf_file = 'rdc-results_structs_first.txt';
ef_file = 'rdc-results_exacts_first.txt';
sef_file = 'rdc-results_structs_exacts_first.txt';

delimiter = '\n';

components = [0 10 50 100];

range0 = [1 0 100 0];
range10 = [102 0 201 0];
range50 = [203 0 302 0];
range100 = [304 0 403 0];

no0 = dlmread(no_file, delimiter', range0);
no10 = dlmread(no_file, delimiter, range10);
no50 = dlmread(no_file, delimiter, range50);
no100 = dlmread(no_file, delimiter, range100);

sf0 = dlmread(sf_file, delimiter, range0);
sf10 = dlmread(sf_file, delimiter, range10);
sf50 = dlmread(sf_file, delimiter, range50);
sf100 = dlmread(sf_file, delimiter, range100);

ef0 = dlmread(ef_file, delimiter, range0);
ef10 = dlmread(ef_file, delimiter, range10);
ef50 = dlmread(ef_file, delimiter, range50);
ef100 = dlmread(ef_file, delimiter, range100);

sef0 = dlmread(ef_file, delimiter, range0);
sef10 = dlmread(ef_file, delimiter, range10);
sef50 = dlmread(ef_file, delimiter, range50);
sef100 = dlmread(ef_file, delimiter, range100);

no = [mean(no0) mean(no10) mean(no50) mean(no100)];
sf = [mean(sf0) mean(sf10) mean(sf50) mean(sf100)];
ef = [mean(ef0) mean(ef10) mean(ef50) mean(ef100)];
sef = [mean(sef0) mean(sef10) mean(sef50) mean(sef100)];

hold all;

h1 = plot(components, 1000*no, '-o');
h2 = plot(components, 1000*sf, '-+');
h3 = plot(components, 1000*ef, '-*');
h4 = plot(components, 1000*sef, '-x');

legend([h1 h2 h3 h4], 'No optimisation', 'Structures first', 'Exacts first', 'Structures-Exacts first', 'Location', 'Best');
title('RDC Schema search for query +S...');
xlabel('Number of other components registered');
ylabel('Time to search through components (milliseconds)');