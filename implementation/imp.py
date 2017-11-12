import numpy as np;
from scipy.sparse import csc_matrix;
from scipy.sparse import diags;

TIMESTEP = 30;
EPSILON = 0.95;
CARLENGTH = 3;
TIMEGAP = 3;
MINGAP = 2;

class Intersection:
    def __init__(self, lat, lon):
        self.lat = lat;
        self.lon = lon;
        self.incoming = [];
        self.outgoing = [];

class Road:
    def __init__(self, num, startpoint, endpoint, length, speed, name):
        self.num = num;
        self.endpoint = endpoint;
        self.startpoint = startpoint;
        self.length = length;
        self.speed = speed;
        self.name = name;
        self.turns = {};
        
        traveltime = length / speed;
        self.leaveratio = min(1, TIMESTEP * EPSILON / traveltime);

def init():
    intersections = {};
    roads = [];

    with open("nodes.txt") as fin:
        for line in fin:
            num, lon, lat = line.split('\t');
            lat = float(lat);
            lon = float(lon);
            intersections[int(num)] = Intersection(lat, lon);

    with open("edges.txt") as fin:
        for line in fin:
            start, fin, length, _ , direction, speed, _ , name = line.split('\t');

            fin = int(fin);
            start = int(start);
            length = float(length);
            direction = int(direction);
            speed = int(speed);
            name = name.strip();

            if direction == 1 or direction == 2:
                road = Road(len(roads), intersections[start], intersections[fin], length, speed, name);
                intersections[start].outgoing.append(road);
                intersections[fin].incoming.append(road);
                roads.append(road);

            if direction == 1:
                road = Road(len(roads), intersections[fin], intersections[start], length, speed, name);
                intersections[fin].outgoing.append(road);
                intersections[start].incoming.append(road);
                roads.append(road);

    for v in intersections.values():
        for incoming in v.incoming:
            count = 0;

            for outgoing in v.outgoing:
                if outgoing.endpoint != incoming.startpoint or len(v.outgoing) == 1:
                    count += 1;

            for outgoing in v.outgoing:
                if outgoing.endpoint != incoming.startpoint or len(v.outgoing) == 1:
                    incoming.turns[outgoing] = incoming.leaveratio / count;
                    #change this for more accuracy
    
    return roads, intersections


roads, intersections = init();
print("starting....");

row = [];
column = [];
data = [];

capacities = np.empty(len(roads));

for road in roads:
    capacities[road.num] = road.length / (CARLENGTH + MINGAP);

    for dest, val in road.turns.items():
        column.append(dest.num);
        row.append(road.num);
        data.append(val);

change_matrix = csc_matrix((data, (row, column)), shape=(len(roads), len(roads)))

state = np.ones(len(roads));

for i in range(10):
    changes = state * change_matrix;
    remaining = np.maximum(capacities - state, 0);
    limits = np.minimum(remaining, TIMESTEP / TIMEGAP);
    with np.errstate(divide='ignore', invalid='ignore'):
        ratios = np.divide(remaining, changes);
        ratios[np.isnan(ratios)] = 0;

    final_changes = change_matrix * diags(np.minimum(ratios, 1));
    sum_changes = final_changes * np.ones(len(roads));
    final_matrix = diags(np.ones(len(roads)) - sum_changes) + final_changes;
    
    state = state * final_matrix;
    print(state);



