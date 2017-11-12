intersections = {};
with open("AUS_ND.txt") as fin, open("nodes.txt", "w") as fout:
    for line in fin:
        num, lon, lat = line.split('\t');
        lat = float(lat);
        lon = float(lon);
        if (lat < -33.75 and lat > -34.037) and lon > 150.582604:
            intersections[int(num)] = 1;
            fout.write(line);

with open("AUS_ST.txt") as fin, open("edges.txt", "w") as fout:

    for line in fin:
        start, fin, length, _ , direction, speed, _ , name = line.split('\t');

        fin = int(fin);
        start = int(start);
        length = float(length);
        direction = int(direction);
        speed = int(speed);
        name = name.strip();

        if (not start in intersections) or (not fin in intersections) or start == fin or (not direction in [1,2]) or speed == 0:
            continue;
        fout.write(line);

