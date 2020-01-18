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
		EventOb event;
		event.id = eventId;
		event.eventType = eType;

		double temp = (float)rand() / (float)RAND_MAX;
		randVar = -log(1.00 - temp) / lambda;

		elapsedTime += randVar;
		event.instTime = elapsedTime;
		randVarArray.push_back(event);

		randValuesArray.push_back(randVar);
		eventId++;
	}
}

// Individual, non-accumulated values
double genRandomValue(int lambda, const double totalSimTime){
	double temp = (float)rand() / (float)RAND_MAX;
	return (-log(1.00 - temp) / (float)lambda);
}


void genObserverEvents(vector<EventOb>& observeEvents, int sampleRate, const double totalSimTime) {
	double elapsedTime = 0.0;
	int eventId = 0;

	while (elapsedTime < totalSimTime) {
		double temp = (float)rand() / (float)RAND_MAX;
		double randVar = -log(1.00 - temp) / sampleRate;
		elapsedTime += randVar;

		EventOb event;
		event.id = eventId;
		event.eventType = EventType::Observer;
		event.instTime = elapsedTime;
		event.packetDropped = false;
		observeEvents.push_back(event);

		eventId++;
	}
}

// Note: I need everything to get scaled up by 1000 or numbers will be too small
void genDepartEvents(vector<double>& arrivalValues, const int serviceRate, vector<EventOb>& departEvents, const double totalSimTime) {

	std::queue<double> departQueue;
	int arrIndex = 0;
	int eventIndex = 0;
	double elapsedTime = 0;

	while (elapsedTime < totalSimTime) {
		
		if (departQueue.empty() || (departQueue.front() > arrivalValues.at(arrIndex))) {
			double timePassed = arrivalValues.at(arrIndex);
			
			if (!departQueue.empty()) {
				departQueue.front() = departQueue.front() - timePassed;
			}

			elapsedTime += timePassed;

			if (elapsedTime >= totalSimTime) break;
			departQueue.push(genRandomValue(serviceRate, totalSimTime));
			arrIndex++;
		}
		else if (departQueue.front() < arrivalValues.at(arrIndex)) {
			double timePassed = departQueue.front();
			elapsedTime += timePassed;

			if (elapsedTime >= totalSimTime) break;
			departQueue.pop();
			arrivalValues.at(arrIndex) -= timePassed;
			
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
	return e1.instTime < e2.instTime;
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

	for (EventOb& event : allEvents) {
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

int main() {
	std::vector<double> arrivalValues;
	std::vector<EventOb> arrivalEvents;
	std::vector<double> departValues;
	std::vector<EventOb> departEvents;
	std::vector<EventOb> observeEvents;

	/* Normal Parameters */
	// Parameters are scaled up by 1000 for convenience
	const int SERVICE_RATE = 1000000;
	const double packLength = 2000.0;
	const int TOTAL_SIMTIME = 1000; // not scaled 
	const int QUEUE_CAPACITY = 10;

	ofstream myfile;
	myfile.open("data/output4.csv");
	myfile << "Row Constant Value, Av. No. Packets in Buffer, Idle Time %" << endl;
	
	for (double ROW_CONST = 0.25; ROW_CONST < 0.3; ROW_CONST += 0.1) {

		double ARRIVAL_RATE = SERVICE_RATE * ROW_CONST / packLength;


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

		/* For Infinite Queue */
		double lambdaForDepartEvents = SERVICE_RATE / packLength;
		genDepartEvents(arrivalValues, lambdaForDepartEvents, departEvents, TOTAL_SIMTIME);

		/* For the Finite Queue */
		//genDepartEvents(arrivalValues, arrivalEvents, lambdaForDepartEvents, departEvents, TOTAL_SIMTIME, QUEUE_CAPACITY);

		genObserverEvents(observeEvents, ARRIVAL_RATE * 6, TOTAL_SIMTIME);


		vector<EventOb> allEvents;
		for (int i = 0; i < arrivalEvents.size(); i++) {
			allEvents.push_back(arrivalEvents[i]);
		}
		int baseIndex = arrivalEvents.size();
		for (int i = 0; i < departEvents.size(); i++) {
			allEvents.push_back(departEvents[i]);
		}
		baseIndex += departEvents.size();
		for (int i = 0; i < observeEvents.size(); i++) {
			allEvents.push_back(observeEvents[i]);
		}

		sort(allEvents.begin(), allEvents.end(), compare);

		// Test that all events are in order
		vector<EventOb>::iterator it = allEvents.begin();
		double previousTime = it->instTime;
		it++;
		for (; it != allEvents.end(); ++it) {
			double currentTime = it->instTime;
			if (previousTime > currentTime) {
				cerr << "Error: Misordering of events in timing" << endl;
				exit(1);
			}
			previousTime = currentTime;
		}

		vector<Statistics> stats = runDESimulator(allEvents);

		// Print statistics
		double idleRatio = 0.0;
		double averagePacketsInQueue = 0.0;
		vector<Statistics>::iterator itStats = stats.end();
		itStats--;
		itStats--;
		const int SAMPLES_COLLECTED = 7;
		for (int i = 0; i < SAMPLES_COLLECTED; i++, itStats--) {
			cout << "idle time: " << itStats->idleTime << endl;
			idleRatio += itStats->idleTime;
			cout << "AVG_PACKETS: " << itStats->avgPacketsInQueue << endl;
			averagePacketsInQueue += itStats->avgPacketsInQueue;
		}


		myfile << ((float)ARRIVAL_RATE / (float)SERVICE_RATE) << ",";
		myfile << (averagePacketsInQueue / (float)SAMPLES_COLLECTED) << ",";
		myfile << (idleRatio / (float)SAMPLES_COLLECTED) << endl;


		arrivalValues.clear();
		arrivalEvents.clear();
		departValues.clear();
		departEvents.clear();
		observeEvents.clear();
		stats.clear();
		allEvents.clear();
	}
	myfile.close();

	//printTest1Results();
	return 0;
}


