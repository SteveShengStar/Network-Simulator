#include <math.h>
#include <stdlib.h>
#include <stdio.h>
#include <iostream>
#include <algorithm>
#include <array>
#include <iterator>
#include <queue>
#include <vector>
#include <fstream>
#include <time.h>
#include <string>

using namespace std;

enum EventType {Arrival, Departure, Observer};
struct EventOb {
	int id;
	EventType eventType;
	double instTime;
	bool packetDropped;
};
struct Statistics {
	double avgPacketsInQueue;
	double idleTime;
	double lossRatio;
};

enum QueueStateLabel {ACTIVE, IDLE};
struct QueueState {
	double idleTime;
	double activeTime;
	QueueStateLabel stateLabel;
};
// Accumulated Values
void genArrivalEvents(vector<double>& randValuesArray, vector<EventOb>& randVarArray, EventType eType, int lambda, const double totalSimTime) {

	double randVar;
	double elapsedTime = 0.0;
	int eventId = 0;

	while (elapsedTime < totalSimTime) {
		int randomTemp = rand();
		if (randomTemp == RAND_MAX) { randomTemp = RAND_MAX - 1; }
		double temp = (float)randomTemp / (float)RAND_MAX;
		randVar = -log(1.00 - temp) / lambda;
		if (isinf(randVar)) {
			cout << "Infinity detected: continue executed in for" << endl;
			continue;
		}

		EventOb event;
		event.id = eventId;
		event.eventType = eType;

		elapsedTime += randVar;
		event.instTime = elapsedTime;

		randVarArray.push_back(event);
		randValuesArray.push_back(randVar);
		eventId++;
	}
}

// Individual, non-accumulated values
double genRandomValue(int lambda, const double totalSimTime){
	double randVar;
	do {
		int randomTemp = rand();
		if (randomTemp == RAND_MAX) { randomTemp = RAND_MAX - 1; }
		double temp = (float)randomTemp / (float)RAND_MAX;
		randVar = -log(1.00 - temp) / (float)lambda;
	} while (isinf(randVar));
	return randVar;
}


void genObserverEvents(vector<EventOb>& observeEvents, int sampleRate, const double totalSimTime) {
	double elapsedTime = 0.0;
	int eventId = 0;

	while (elapsedTime < totalSimTime) {
		int randomTemp = rand();
		if (randomTemp == RAND_MAX) { randomTemp = RAND_MAX - 1; }
		double temp = (float)randomTemp / (float)RAND_MAX;
		double randVar = -log(1.00 - temp) / sampleRate;
		if (isinf(randVar)) {
			cout << "Infinity detected: continue executed in for" << endl;
			continue;
		}

		elapsedTime += randVar;

		EventOb event;
		event.id = eventId;
		event.eventType = EventType::Observer;
		event.instTime = elapsedTime;
		event.packetDropped = false;
		observeEvents.push_back(event);

		eventId++;
	}
	//cout << observeEvents.back().instTime << endl;
}

// Note: I need everything to get scaled up by 1000 or numbers will be too small
void genDepartEvents(vector<double>& arrivalValues, const int serviceRate, vector<EventOb>& departEvents, const double totalSimTime) {

	std::queue<double> departQueue;
	vector<double>::iterator arrivalIterator = arrivalValues.begin();
	int eventIndex = 0;
	double elapsedTime = 0;
	int arrIndex = 0;
	int departCount = 0;

	while (elapsedTime < totalSimTime && arrivalIterator != arrivalValues.end()) {

		if (departQueue.empty() || (departQueue.front() > *arrivalIterator)) {
			double timePassed = *arrivalIterator;
			
			if (!departQueue.empty()) {
				departQueue.front() = departQueue.front() - timePassed;
			}

			elapsedTime += timePassed;

			if (elapsedTime >= totalSimTime) break;
			departQueue.push(genRandomValue(serviceRate, totalSimTime));
			arrivalIterator++;
			arrIndex++;
		}
		else if (departQueue.front() < *arrivalIterator) {
			double timePassed = departQueue.front();
			if (timePassed < 0) {
				cout << "Negative Value." << endl;
			}
			elapsedTime += timePassed;

			if (elapsedTime >= totalSimTime) break;
			departQueue.pop();
			(*arrivalIterator) -= timePassed;
			
			EventOb event;
			event.id = eventIndex;
			event.instTime = elapsedTime;
			event.eventType = EventType::Departure;
			event.packetDropped = false;
			departEvents.push_back(event);

			eventIndex++;
			departCount++;
			if (arrIndex < departCount) {
				cout << arrIndex << endl;
				cout << "Error happens here: " << endl;
			}
		}
		else {
			double timePassed = departQueue.front();
			elapsedTime += timePassed;

			if (elapsedTime >= totalSimTime) break;
			departQueue.pop();

			EventOb event;
			event.id = eventIndex;
			event.instTime = elapsedTime;
			event.eventType = EventType::Departure;
			event.packetDropped = false;
			departEvents.push_back(event);
			eventIndex++;

			departQueue.push(genRandomValue(serviceRate, totalSimTime));
			arrivalIterator++;
			arrIndex++;
			departCount++;
		}
	}
}


void genDepartEvents(vector<double>& arrivalValues, vector<EventOb>& arrivalEvents, const int serviceRate, vector<EventOb>& departEvents, const double totalSimTime, const int capacity) {

	std::queue<double> departQueue;
	int arrIndex = 0;
	int eventIndex = 0;
	double elapsedTime = 0;

	while (elapsedTime < totalSimTime) {

		if (departQueue.empty() || (departQueue.front() > arrivalValues.front()) ) {
			double timePassed = arrivalValues.front();
			arrivalValues.erase(arrivalValues.begin());

			if (!departQueue.empty()) {
				departQueue.front() = departQueue.front() - timePassed;
			}

			elapsedTime += timePassed;

			if (elapsedTime >= totalSimTime) break;
			if (departQueue.size() + 1 > capacity) {
				// Drop the packet
				arrivalEvents.at(arrIndex).packetDropped = true;
			} else {
				departQueue.push(genRandomValue(serviceRate, totalSimTime));
			}
			arrIndex++;
		}
		else if (departQueue.front() < arrivalValues.front()) {
			double timePassed = departQueue.front();
			elapsedTime += timePassed;

			if (elapsedTime >= totalSimTime) break;
			departQueue.pop();
			arrivalValues.front() -= timePassed;

			EventOb event;
			event.id = eventIndex;
			event.instTime = elapsedTime;
			event.eventType = EventType::Departure;
			event.packetDropped = false;

			departEvents.push_back(event);

			eventIndex++;
		}
		else {
			double timePassed = departQueue.front();
			elapsedTime += timePassed;

			if (elapsedTime >= totalSimTime) break;
			departQueue.pop();

			EventOb event;
			event.id = eventIndex;
			event.instTime = elapsedTime;
			event.eventType = EventType::Departure;
			event.packetDropped = false;
			departEvents.push_back(event);
			eventIndex++;

			departQueue.push(genRandomValue(serviceRate, totalSimTime));
			arrIndex++;
		}
	}
}


bool compare(const EventOb& e1, const EventOb& e2) {
	return (e1.instTime < e2.instTime);
}

vector<Statistics> runDESimulator(vector<EventOb> allEvents) {
	
	int arrivals = 0;
	int departures = 0;
	int observations = 0;

	queue<EventOb> eventQueue;
	vector<Statistics> stats;

	QueueState queueState;
	queueState.idleTime = 0;
	queueState.activeTime = 0;
	queueState.stateLabel = QueueStateLabel::IDLE;
	double lastTimeCheckpoint = 0;


	int sumOfPacketsInQueueAllFrames = 0;

	cout << "No. Arrivals, No. Departures, No. Observers -- Before exiting simulator" << endl;
	for (EventOb& event : allEvents) {
		if (isinf(event.instTime)) {
			cout << arrivals << "," << departures << "," << observations << endl;
			//break;
		}
		switch (event.eventType) {
			case EventType::Arrival:
				if (eventQueue.empty()) {
					queueState.idleTime += event.instTime - lastTimeCheckpoint;
					queueState.stateLabel = QueueStateLabel::ACTIVE;
					lastTimeCheckpoint = event.instTime;
				}
				eventQueue.push(event);
				arrivals++;
				break;
			case EventType::Departure:
				departures++;
				if (eventQueue.empty()) { 
					cout << "Event Queue detected as empty" << endl;
					cout << "Departure No.: " << departures << endl;
					continue;
				}
				eventQueue.pop();

				if (eventQueue.empty()) {
					queueState.activeTime += event.instTime - lastTimeCheckpoint;
					queueState.stateLabel = QueueStateLabel::IDLE;
					lastTimeCheckpoint = event.instTime;
				}

				break;
			default:
				observations++;
				sumOfPacketsInQueueAllFrames += eventQueue.size();

				if (queueState.stateLabel == QueueStateLabel::IDLE) {
					queueState.idleTime += event.instTime - lastTimeCheckpoint;
				} else {
					queueState.activeTime += event.instTime - lastTimeCheckpoint;
				}
				lastTimeCheckpoint = event.instTime; 

				Statistics stat;
				stat.avgPacketsInQueue = (float)sumOfPacketsInQueueAllFrames / (float)observations;
				stat.idleTime = queueState.idleTime / event.instTime;
				stat.lossRatio = -1.0; // TODO: no-value ?
				stats.push_back(stat);
		}
	}
	return stats;
}

// Not tested
vector<Statistics> runDESimulatorFiniteBuffer(vector<EventOb> allEvents, int capacity) {

	int arrivals = 0;
	int departures = 0;
	int observations = 0;

	queue<EventOb> eventQueue;
	vector<Statistics> stats;

	QueueState queueState;
	queueState.idleTime = 0;
	queueState.activeTime = 0;
	queueState.stateLabel = QueueStateLabel::IDLE;
	double lastTimeCheckpoint = 0;
	int droppedPackets = 0;


	int sumOfPacketsInQueueAllFrames = 0;

	for (EventOb& event : allEvents) {
		switch (event.eventType) {
		case EventType::Arrival:
			if (eventQueue.empty()) {
				queueState.idleTime += event.instTime - lastTimeCheckpoint;
				queueState.stateLabel = QueueStateLabel::ACTIVE;
				lastTimeCheckpoint = event.instTime;
			} 
			if (event.packetDropped == true) {
				droppedPackets++;
			} else {
				eventQueue.push(event);
			}
			// TODO: remove
			if (eventQueue.size() > capacity) {
				cerr << "Error: Number packets exceeds buffer cap." << endl;
				exit(1);
			}
			arrivals++;
			break;
		case EventType::Departure:
			eventQueue.pop();
			departures++;

			if (eventQueue.empty()) {
				queueState.activeTime += event.instTime - lastTimeCheckpoint;
				queueState.stateLabel = QueueStateLabel::IDLE;
				lastTimeCheckpoint = event.instTime;
			}
			break;
		default:
			observations++;
			sumOfPacketsInQueueAllFrames += eventQueue.size();


			if (queueState.stateLabel == QueueStateLabel::IDLE) {
				queueState.idleTime += event.instTime - lastTimeCheckpoint;
			}
			else {
				queueState.activeTime += event.instTime - lastTimeCheckpoint;
			}
			lastTimeCheckpoint = event.instTime;

			Statistics stat;
			stat.avgPacketsInQueue = (float)sumOfPacketsInQueueAllFrames / (float)observations;
			stat.idleTime = queueState.idleTime / event.instTime;
			stat.lossRatio = (float) droppedPackets / (float) arrivals;
			stats.push_back(stat);
		}
	}
	return stats;
}

/*void printTest1Results(vector<EventOb> someArray) {
	double sum = 0;
	double average;
	double numerator = 0;

	for (EventOb )

	average = sum / 1000;
	cout << "Average: " << average << endl;

	for (int i = 0; i < 1000; i++) {
		numerator += pow(someArray[i] - average, 2);
	}
	double variance = numerator / 999;
	cout << "Variance: " << variance << endl;
}*/
string printEventType(EventOb event) {
	switch (event.eventType) {
	case EventType::Arrival:
		return "Arrival";
		break;
	case EventType::Departure:
		return "Departure";
		break;
	case EventType::Observer:
		return "Observation";
		break;
	}
}

int main() {

	srand(time(0));

	std::vector<double> arrivalValues;
	std::vector<EventOb> arrivalEvents;
	std::vector<EventOb> departEvents;
	std::vector<EventOb> observeEvents;

	/* Normal Parameters */
	// Parameters are scaled up by 1000 for convenience
	const int SERVICE_RATE = 1000000;
	const double packLength = 2000.0;
	const int TOTAL_SIMTIME = 1000; // not scaled 
	const int QUEUE_CAPACITY = 10;

	ofstream myfile;
	myfile.open("data/output7.csv");
	myfile << "Row Constant Value, Av. No. Packets in Buffer, Idle Time %" << endl;
	
	for (double ROW_CONST = 0.25; ROW_CONST < 1; ROW_CONST += 0.1) {
		double ARRIVAL_RATE = SERVICE_RATE * ROW_CONST / packLength;
		cout << "ROW_CONST: " << ROW_CONST << endl;

		/* Test Case 1: Arrival Significantly Faster than Departure */
		//int arrivalRate = 25;
		//int serviceRate = 5;
		// Result: was good

		/* Test Case 2: Departure Significantly Slower than Arrival */
		/*int arrivalRate = 5;
		int serviceRate = 50;*/
		// Result: was good

		genArrivalEvents(arrivalValues, arrivalEvents,
			EventType::Arrival, ARRIVAL_RATE, TOTAL_SIMTIME);

		// double-check sum
		/*double theSum = 0.0;
		for (int i = 0; i < arrivalValues.size() - 1; i++) {
			theSum += arrivalValues.at(i);
			//cout << theSum << endl;
		}*/
		//cout << theSum / arrivalValues.size() << endl;

		/* For Infinite Queue */
		double lambdaForDepartEvents = SERVICE_RATE / packLength;
		genDepartEvents(arrivalValues, lambdaForDepartEvents, departEvents, TOTAL_SIMTIME);
		cout << "Number of depart events: " << departEvents.size() << endl;

		/* For the Finite Queue */
		//genDepartEvents(arrivalValues, arrivalEvents, lambdaForDepartEvents, departEvents, TOTAL_SIMTIME, QUEUE_CAPACITY);

		//genObserverEvents(observeEvents, ARRIVAL_RATE * 6, TOTAL_SIMTIME);


		vector<EventOb> allEvents(arrivalEvents);
		vector<EventOb>::iterator evIterator = allEvents.end();
		allEvents.insert(evIterator, departEvents.begin(), departEvents.end());
		//evIterator = allEvents.end();
		//allEvents.insert(evIterator, observeEvents.begin(), observeEvents.end());
		
		sort(allEvents.begin(), allEvents.end(), compare);


		vector<EventOb>::iterator prev = allEvents.begin();
 		vector<EventOb>::iterator current = allEvents.begin();
		current++;
		while (current != allEvents.end()) {
			// Actual Logic
			if (prev->instTime == current->instTime) {
				if (prev->eventType == EventType::Departure) {
					swap(*prev, *current);
				}
			}
			prev++;
			current++;
			
			// Test that all events are in order
			/*double currentTime = it->instTime;
			if (previousTime > currentTime) {
				cerr << "Error: Misordering of events in timing" << endl;
				exit(1);
			}
			if (previousTime == currentTime) {
				cout << "Found Equal Times: " << endl;
				cout << "Previous Event: ";
				printEventType(previousEvent);
				cout << endl;
				cout << "Current Time: ";
				printEventType(*it);
				cout << endl;
			}
			previousEvent = *it;
			previousTime = currentTime;*/
		}

		int arrivalCount = 0;
		int departCount = 0;
		prev = allEvents.begin();
		current = allEvents.begin();
		current++;
		while (current != allEvents.end()) {
			// Just for Debugging
			switch (current->eventType) {
			case EventType::Arrival:
				arrivalCount++;
				break;
			case EventType::Departure:
				departCount++;
				break;
			}
			if (departCount > arrivalCount) {
				cout << "CurrentEvent Time: " << current->instTime << endl;
				cout << "CurrentEvent Event: " << printEventType(*current) << endl;
				cout << "Previous Event: " << prev->instTime << endl;
				cout << "Previous Event: " << printEventType(*prev) << endl;
			}
			current++;
			prev++;
		}

		/*vector<Statistics> stats = runDESimulator(allEvents);

		// Print statistics
		double idleRatio = 0.0;
		double averagePacketsInQueue = 0.0;
		vector<Statistics>::iterator itStats = stats.end();
		itStats--;
		const int SAMPLES_COLLECTED = 4;
		for (int i = 0; i < SAMPLES_COLLECTED; i++, itStats--) {
			if (isinf(itStats->idleTime) || itStats->idleTime == 0) {
				cerr << "Error: bad idleTime value: " << itStats->idleTime << endl;
				exit(1);
			}
			idleRatio += itStats->idleTime;
			if (isinf(itStats->avgPacketsInQueue) || itStats->avgPacketsInQueue == 0) {
				cerr << "Error: bad idleTime value: " << itStats->avgPacketsInQueue << endl;
				exit(1);
			}
			averagePacketsInQueue += itStats->avgPacketsInQueue;
		}


		myfile << ROW_CONST << ",";
		myfile << (averagePacketsInQueue / (float)SAMPLES_COLLECTED) << ",";
		myfile << (idleRatio / (float)SAMPLES_COLLECTED) << endl;*/

		arrivalValues.clear();
		arrivalEvents.clear();
		departEvents.clear();
		observeEvents.clear();
		//stats.clear();
		allEvents.clear();
	}
	myfile.close();

	return 0;
}


// Testing swapping
//int main() {
//	// attempt 1
//	std::vector<int> v;
//	v.push_back(22);
//	v.push_back(33);
//	v.push_back(44);
//	v.push_back(55);
//	v.push_back(66);
//	v.push_back(77);
//	v.push_back(88);
//	std::vector<int>::iterator prev = v.begin();
//	prev++;
//	std::vector<int>::iterator current = v.begin();
//	current++;
//	current++;
//	current++;
//
//	swap(*prev, *current);
//	//cout << *prev << endl;
//	//cout << *current << endl;
//
//	for (; prev != v.end(); prev++) {
//		cout << *prev << endl;
//	}
//	exit(0);
//}


