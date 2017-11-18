#include <fstream>
#include <iostream>
#include <vector>
#include <unordered_map>
#include <map>
#include <utility>
#include <memory>
#include <algorithm>
#include <set>
#include <iterator>
#include <random>
#include <queue>
#include <string>
#include <cassert>

using namespace std;

unsigned TIMESTEP = 30;
unsigned CARLENGTH = 3;
unsigned MINGAP = 3;
unsigned REACTION = 2;

struct edge;
struct node;
struct car;

template<typename T>
class queue2 : public queue<T> {
	public:
		T &top() {
			return this->front();
		}
};

using car_priority_queue = priority_queue<pair<unsigned int, car*>, vector<pair<unsigned int, car*>>, greater<pair<unsigned int, car*>>>;
using road_priority_queue = priority_queue<pair<unsigned int, edge*>, vector<pair<unsigned int, edge*>>, greater<pair<unsigned int, edge*>>>;
using road_queue = queue2<pair<unsigned int, edge*>>;

struct car {
	car(edge *at) : at_{at} {}
	edge *at_;
};

struct edge {
	double lat_;
	double lon_;
	unsigned long long start_;
	unsigned long long end_;
	long long id_;
	double length_;
	unsigned speed_;

	unsigned count_;
	bool cooldown_;

	vector<edge*> turns_;
	queue<car*> inqueue_;

	edge(double lat, double lon, unsigned long long start, unsigned long long end, long long id, double len, unsigned speed) 
		: lat_{lat}, lon_{lon}, start_{start}, end_{end}, id_{id}, length_{len}, speed_{speed}, count_{0}, cooldown_{false}  {}

	unsigned capacity() {
		return length_ / (CARLENGTH + MINGAP);
	}

	unsigned timetotraverse() {
		return 3.6 * length_ / speed_;
	}

	unsigned gaptime() {
		return (count_ - 1) * REACTION;
	}

	bool atcapacity() {
		return count_ > capacity();
	}

	void acceptone(road_queue &cdq, road_priority_queue &gpq, car_priority_queue &cpq, unsigned time) {
		if(!atcapacity() && !cooldown_ && !inqueue_.empty()){
			auto tmp = inqueue_.front();
			inqueue_.pop();

			gpq.emplace(time + tmp->at_->gaptime(), tmp->at_);
			cpq.emplace(time + timetotraverse(), tmp);

			count_++;
			cooldown_ = true;
			cdq.emplace(time + 3, this);
			tmp->at_ = this;
		}
	}
};

struct node {
	double lat_;
	double lon_;
	vector<edge*> incoming_;
	vector<edge*> outgoing_;

	friend istream& operator>> (istream& in, node &n){
		return in >> n.lon_ >> n.lat_;
	}
};


istream& operator>> (istream& in, pair<unsigned long long, node> &n){
	in >> n.first >> n.second >> ws;
	in.peek();
	return in;
}

vector<unique_ptr<edge>> init(){

	using iiterator = istream_iterator<pair<unsigned long long, node>>;

	unordered_map<unsigned long long, node> intersections;
	
	ifstream fin("nodes.txt");
	std::copy(iiterator(fin), iiterator(), inserter(intersections, intersections.end()));

	fin.close();
	fin.open("edges.txt");

	vector<unique_ptr<edge>> roads;

	while(!fin.eof()){
		unsigned long long start, end;
		double length; 
		int dir;
		int speed;
		long long id, type;
		char buf[250];

		fin >> start >> end >> length >> id >> dir >> speed >> type;
		fin.getline(buf, 250);
		fin.peek();

		if(dir == 1 || dir == 2){
			double lat = (intersections[start].lat_ + intersections[end].lat_ * 2) / 3;
			double lon = (intersections[start].lon_ + intersections[end].lon_ * 2) / 3;

			roads.emplace_back(make_unique<edge>(lat, lon, start, end, id, length, speed));
			intersections[start].outgoing_.push_back(roads.back().get());
			intersections[end].incoming_.push_back(roads.back().get());
		}

		if(dir == 1){
			double lat = (intersections[start].lat_ * 2 + intersections[end].lat_) / 3;
			double lon = (intersections[start].lon_ * 2 + intersections[end].lon_) / 3;

			roads.emplace_back(make_unique<edge>(lat, lon, end, start, -id, length, speed));
			intersections[end].outgoing_.push_back(roads.back().get());
			intersections[start].incoming_.push_back(roads.back().get());
		}
	}

	fin.close();

	for(const auto &entry : intersections) {
		if(!entry.second.outgoing_.size() && !entry.second.incoming_.empty()) {
			auto &incoming = entry.second.incoming_.front();
			double lat = (intersections[incoming->start_].lat_ * 2 + intersections[incoming->end_].lat_) / 3;
			double lon = (intersections[incoming->start_].lon_ * 2 + intersections[incoming->end_].lon_) / 3;
			roads.emplace_back(make_unique<edge>(lat, lon, incoming->end_, incoming->start_, -incoming->id_, incoming->length_, incoming->speed_));
			intersections[incoming->end_].outgoing_.push_back(roads.back().get());
			intersections[incoming->start_].incoming_.push_back(roads.back().get());
		}
	}

	for(const auto &entry : intersections)
	for(const auto &incoming : entry.second.incoming_) 
	for(const auto &outgoing : entry.second.outgoing_) {
		if(outgoing->id_ != -incoming->id_ || entry.second.outgoing_.size() == 1){
			incoming->turns_.push_back(outgoing);
		}
	}

	return roads;
}

bool compare(double lat, double lon, edge &a, edge &b){
	if(a.atcapacity()) {
		return false;
	}
	if(b.atcapacity()) {
		return true;
	}
	double alat = (a.lat_ - lat);
	double alon = (a.lon_ - lon);
	double blat = (b.lat_ - lat);
	double blon = (b.lon_ - lon);

	return alat * alat + alon * alon < blat * blat + blon * blon;
}

vector<unique_ptr<car>> populate(vector<unique_ptr<edge>> &roads) {
	double mlat = -33.892784,  mlon = 151.157815;

	vector<unique_ptr<car>> cars;
	normal_distribution<double> norm(0, 0.01);
	default_random_engine generator;
	for(int i = 0; i < 10000; i++) {
		double lat = mlat + norm(generator);
		double lon = mlon + norm(generator);

		auto &mine = *min_element(roads.begin(), roads.end(), [&lat, &lon] (auto &a, auto &b) { return compare(lat, lon, *a, *b); });

		cars.push_back(make_unique<car>(mine.get()));
		mine->count_++;
		mine->turns_[generator() % mine->turns_.size()]->inqueue_.push(cars.back().get());
	}
	return cars;
}

template<typename q1, typename q2>
bool sooner(q1 &a, q2 &b) {
	if(a.empty()){
		return false;
	}
	if(b.empty()){
		return true;
	}
	return a.top().first < b.top().first;
}

void record_snapshot(unsigned time, unsigned size, vector<unique_ptr<car>> &cars){
	cout << time << endl;
	map<long long, unsigned> counts;
	for_each(cars.begin(), cars.end(), [&counts](auto &c) { counts[c->at_->id_]++; });

	ofstream fout("data/" + to_string(time));
	assert(fout.is_open());
	for(auto &pair: counts) {
		fout << pair.first << ' ' << pair.second  << '\n';
	}
	fout.close();
}

int main() {
	cout.precision(10);
	auto roads = init();
	auto cars = populate(roads);

	road_priority_queue gpq;
	car_priority_queue cpq;
	road_queue cdq;
	default_random_engine generator;

	for_each(roads.begin(), roads.end(), [&](auto &road) { road->acceptone(cdq, gpq, cpq, 0); });

	unsigned t = 0, o = 0;
	while(t < 10000){
		if(gpq.empty() && cpq.empty() && cdq.empty()) {
			cerr << "something fucked up :(" << endl;
			throw runtime_error("something fucked up :(");
		}
		if(sooner(gpq, cpq) && sooner(gpq, cdq)){
			t = gpq.top().first;
			if(t != o && t % 60 == 0){ record_snapshot(t, roads.size(), cars); }

			auto cur = gpq.top().second;
			gpq.pop();

			cur->count_--;
			cur->acceptone(cdq, gpq, cpq, t);
		}
		else if(!sooner(gpq, cpq) && sooner(cpq, cdq)){
			t = cpq.top().first;
			if(t != o && t % 60 == 0){ record_snapshot(t, roads.size(), cars); }

			auto cur = cpq.top().second;
			cpq.pop();

			//make turn
			auto turn = cur->at_->turns_[generator() % cur->at_->turns_.size()];	
			turn->inqueue_.push(cur);
			turn->acceptone(cdq, gpq, cpq, t);
		}
		else {
			t = cdq.front().first;
			if(t != o && t % 60 == 0){ record_snapshot(t, roads.size(), cars); }

			auto cur = cdq.front().second;
			cdq.pop();

			cur->cooldown_ = false;
			cur->acceptone(cdq, gpq, cpq, t);
		}
		o = t;
	}
}
