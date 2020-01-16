#include <math.h>
#include <stdlib.h>
#include <stdio.h>
#include <iostream>
#include <algorithm>
#include <array>
#include <iterator>
#include <queue>
#include <vector>

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
		randVarArray[eventId].id = eventId;
		randVarArray[eventId].eventType = eType;

		double temp = (float)rand() / (float)RAND_MAX;
		randVar = -log(1.00 - temp) / lambda;

		elapsedTime += randVar;
		randVarArray[eventId].instTime = elapsedTime;
		randValuesArray[eventId] = randVar;
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

int main() {
	std::vector<double> arrivalValues;
	std::vector<EventOb> arrivalEvents;
	std::vector<double> departValues;
	std::vector<EventOb> departEvents;
	std::vector<EventOb> observeEvents;

	/* Normal Parameters */
	const int SERVICE_RATE = 25;
	const int ARRIVAL_RATE = 75;
	const int TOTAL_SIMTIME = 1000;
	const int QUEUE_CAPACITY = 10;

	/* Test Case 1: Arrival Significantly Faster than Departure */
	//int arrivalRate = 25;
	//int serviceRate = 5;
	// Result: was good
	
	/* Test Case 2: Departure Significantly Slower than Arrival */
	/*int arrivalRate = 5;
	int serviceRate = 50;*/
	// Result: was good

	double queueUtilization = (float)ARRIVAL_RATE / (float)SERVICE_RATE;

	genArrivalEvents(arrivalValues, arrivalEvents, 
			EventType::Arrival, ARRIVAL_RATE, TOTAL_SIMTIME);

	/* For Infinite Queue */
	genDepartEvents(arrivalValues, SERVICE_RATE, departEvents, TOTAL_SIMTIME);

	/* For the Finite Queue */
	//genDepartEvents(arrivalValues, arrivalEvents, SERVICE_RATE, departEvents, TOTAL_SIMTIME, QUEUE_CAPACITY);

	genObserverEvents(observeEvents, ARRIVAL_RATE*6, TOTAL_SIMTIME);


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

	runDESimulator(allEvents);

	// Test that all events are in order
	// Result: No Errors Here so far.
	for (int i = 0; i < allEvents.size() - 1; i++) {
		if (allEvents[i].instTime >= allEvents[i+1].instTime) {
			cerr << "Error: Misordering of events in timing" << endl;
			exit(1);
		}
	}
	return 0;
}


