import numpy as np;
import matplotlib.pyplot as plt;
from scipy.sparse import csc_matrix;
from scipy.sparse import diags;

EPSILON = 0.9;
CARLENGTH = 5;
TIMEGAP = 3;
MINGAP = 3;
START = 60;
MOD = 60;
END = 3600;

class Intersection:
    def __init__(self, lat, lon):
        self.lat = lat;
        self.lon = lon;
        self.incoming = [];
        self.outgoing = [];

class Road:
    def __init__(self,n2, num, startpoint, endpoint, length, speed):
        self.n2 = n2;
        self.num = num;
        self.endpoint = endpoint;
        self.startpoint = startpoint;
        self.length = length;
        self.speed = speed;
        self.turns = {};
        
        self.traveltime = length / speed;
    def leaveratio(self, timestep):
        return min(1, timestep * EPSILON / self.traveltime);


def init():
    intersections = {};
    roads = [];
    mappings = {};

    with open("nodes.txt") as fin:
        for line in fin:
            num, lon, lat = line.split('\t');
            lat = float(lat);
            lon = float(lon);
            intersections[int(num)] = Intersection(lat, lon);

    with open("edges.txt") as fin:
        for line in fin:
            start, fin, length, num , direction, speed, _ , _ = line.split('\t');

            fin = int(fin);
            start = int(start);
            length = float(length);
            direction = int(direction);
            speed = int(speed);
            num = int(num);

            if direction == 1 or direction == 2:
                road = Road(num, len(roads), intersections[start], intersections[fin], length, speed);
                intersections[start].outgoing.append(road);
                intersections[fin].incoming.append(road);
                mappings[num] = len(roads);
                roads.append(road);

            if direction == 1:
                road = Road(-num, len(roads), intersections[fin], intersections[start], length, speed);
                intersections[fin].outgoing.append(road);
                intersections[start].incoming.append(road);
                mappings[-num] = len(roads);
                roads.append(road);

    for v in intersections.values():
        if len(v.outgoing) == 0 and len(v.incoming) > 0:
            incoming = v.incoming[0];
            road = Road(-incoming.n2, len(roads), incoming.endpoint, incoming.startpoint, incoming.length, incoming.speed);
            incoming.endpoint.outgoing.append(road);
            incoming.startpoint.incoming.append(road);
            mappings[-incoming.n2] = len(roads);
            roads.append(road);


    for v in intersections.values():
        for incoming in v.incoming:
            count = 0;

            for outgoing in v.outgoing:
                if outgoing.n2 != -incoming.n2 or len(v.outgoing) == 1:
                    count += 1;

            for outgoing in v.outgoing:
                if outgoing.n2 != -incoming.n2 or len(v.outgoing) == 1:
                    incoming.turns[outgoing] = 1 / count;
                    #change this for more accuracy

    return roads, mappings;


roads, mappings = init();
print("starting....");

cols = ['bo', 'gv', 'r^', 'cs', 'm*', 'yo', 'ko', 'wo']
plt.axis([0, 3600, 130, 400])
plt.ylabel("Accuracy (L2 error)")
plt.xlabel("Simulated Time (s)")
x = range(60, 3600, 60);

capacities = np.empty(len(roads));
for road in roads:
    capacities[road.num] = road.length / (CARLENGTH + MINGAP);

for ind, timestep in enumerate([0, 1, 15, 30, 60]):
    row = [];
    column = [];
    data = [];

    for road in roads:
        for dest, val in road.turns.items():
            column.append(dest.num);
            row.append(road.num);
            data.append(val * road.leaveratio(timestep));

    change_matrix = csc_matrix((data, (row, column)), shape=(len(roads), len(roads)))

    state = np.zeros(len(roads));
    with open("data/" + str(START)) as fin:
        for line in fin:
            num, count = line.split();
            state[mappings[int(num)]] = int(count);

    print(timestep);
    if timestep == 0:
        timestep = 60;

    datapoints = [];

    for t in range(START + timestep, END + timestep, timestep):
        changes = state * change_matrix;
        remaining = np.maximum(capacities - state, 0);
        limits = np.minimum(remaining, timestep / TIMEGAP);
        with np.errstate(divide='ignore', invalid='ignore'):
            ratios = np.divide(limits, changes);
            ratios[np.isnan(ratios)] = 0;

        final_changes = change_matrix * diags(np.minimum(ratios, 1));
        sum_changes = final_changes * np.ones(len(roads));
        final_matrix = diags(np.ones(len(roads)) - sum_changes) + final_changes;
        
        state = state * final_matrix;
        if t % MOD == 0:
            comp = np.zeros(len(roads));
            with open("data/" + str(t)) as fin:
                for line in fin:
                    num, count = line.split();
                    comp[mappings[int(num)]] = int(count);
            
            datapoints.append(np.linalg.norm(state - comp));


    plt.plot(x, datapoints, cols[ind], label=str(timestep) + 's timestep' if ind > 0 else 'control');

plt.legend(loc='upper left')
plt.show();
