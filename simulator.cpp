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
void genRandomValues(vector<double>& randomValArray, int lambda, const double totalSimTime){
	double elapsedTime = 0.0;

	while (elapsedTime < totalSimTime) {
		double temp = (float)rand() / (float)RAND_MAX;
		double randVar = -log(1.00 - temp) / lambda;
	
		randomValArray.push_back(randVar);
		elapsedTime += randVar;
	}
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
		observeEvents.push_back(event);

		eventId++;
	}
}

void genDepartEvents(vector<double>& arrivalValues, vector<double>& departValues, vector<EventOb>& departEvents, const double totalSimTime) {

	std::queue<double> departQueue;
	int arrIndex = 0;
	int departIndex = 0;
	double elapsedTime = 0;

	while (elapsedTime < totalSimTime) {
		
		if (departQueue.empty() || (departQueue.front() > arrivalValues.at(arrIndex))) {
			double timePassed = arrivalValues.at(arrIndex);
			
			if (!departQueue.empty()) {
				departQueue.front() = departQueue.front() - timePassed;
			}

			elapsedTime += timePassed;

			if (elapsedTime >= totalSimTime) break;
			departQueue.push(departValues.at(arrIndex));
			arrIndex++;
		}
		else if (departQueue.front() < arrivalValues.at(arrIndex)) {
			double timePassed = departQueue.front();
			elapsedTime += timePassed;

			if (elapsedTime >= totalSimTime) break;
			departQueue.pop();
			arrivalValues.at(arrIndex) -= timePassed;
			
			EventOb event;
			event.id = departIndex;
			event.instTime = elapsedTime;
			event.eventType = EventType::Departure;
			departEvents.push_back(event);

			departIndex++;
		} 
	}
}

/*
template<std::size_t SIZE1, std::size_t SIZE2, std::size_t SIZE3>
void genDepartEvents(std::array<double, SIZE1>& arrivalValues, std::array<double, SIZE2>& departValues, std::array<EventOb, SIZE3>& departEvents, int capacity) {

	std::queue<double> departQueue;
	int arrIndex = 0;
	int departIndex = 0;
	double elapsedTime = 0;

	departQueue.push(departValues[arrIndex]);
	elapsedTime += arrivalValues[arrIndex];
	arrIndex++;

	while (arrIndex < arrivalValues.size()) {

		if (departQueue.empty() || (departQueue.front() > arrivalValues[arrIndex])) {
			double timePassed = arrivalValues[arrIndex];

			if (!departQueue.empty()) {
				departQueue.front() = departQueue.front() - timePassed;
			}

			elapsedTime += timePassed;
			departQueue.push(departValues[arrIndex]);
			arrIndex++;
		}
		else if (departQueue.front() < arrivalValues[arrIndex]) {
			double timePassed = departQueue.front();
			elapsedTime += timePassed;
			departQueue.pop();
			arrivalValues[arrIndex] -= timePassed;

			departEvents[departIndex].id = departIndex;
			departEvents[departIndex].instTime = elapsedTime;
			departEvents[departIndex].eventType = EventType::Departure;
			departIndex++;
		}
	}

	while (!departQueue.empty()) {
		if (departIndex >= SIZE1) {
			cerr << "Error: Depart Index Exceeded 1000" << endl;
		}
		double timePassed = departQueue.front();
		elapsedTime += timePassed;

		departQueue.pop();
		departEvents[departIndex].id = departIndex;
		departEvents[departIndex].instTime = elapsedTime;
		departEvents[departIndex].eventType = EventType::Departure;
		departIndex++;
	}
}
*/

bool compare(const EventOb& e1, const EventOb& e2) {
	return e1.instTime < e2.instTime;
}

template<std::size_t SIZE>
void runDESimulator(array<EventOb, SIZE> allEvents) {
	
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
				eventQueue.add(event);
				arrivals++;
				break;
			case EventType::Departure:
				if (event.id == eventQueue.front().id) {
					eventQueue.pop();
					departures++;

					if (eventQueue.empty()) {
						queueState.activeTime += event.instTime - lastTimeCheckpoint;
						queueState.stateLabel = QueueStateLabel::IDLE;
						lastTimeCheckpoint = event.instTime;
					}

				} else {
					throw Exception("Error: in DES, Mismatch between allEvents and eventQueue event id"); // TODO
					exit(1);
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
}

// Not sure about this anymore !
template<std::size_t SIZE>
void runDESimulatorFiniteBuffer(array<EventOb, SIZE> allEvents, int capacity) {

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
			if (eventQueue.size() + 1 > capacity) {
				droppedPackets++;
			} else {
				eventQueue.add(event);
			}
			arrivals++;
			break;
		case EventType::Departure:
			if (event.id == eventQueue.front().id) {
				eventQueue.pop();
				departures++;

				if (eventQueue.empty()) {
					queueState.activeTime += event.instTime - lastTimeCheckpoint;
					queueState.stateLabel = QueueStateLabel::IDLE;
					lastTimeCheckpoint = event.instTime;
				}

			}
			else {
				throw Exception("Error: in DES, Mismatch between allEvents and eventQueue event id"); // TODO
				exit(1);
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
}

int main() {
	const int SAMPLE_SIZE = 1000;
	std::vector<double> arrivalValues;
	std::vector<EventOb> arrivalEvents;
	std::vector<double> departValues;
	std::vector<EventOb> departEvents;
	std::vector<EventOb> observeEvents;

	/* Normal Parameters */
	int serviceRate = 25;
	int arrivalRate = 75;
	int queueCapacity;

	/* Test Case 1: Arrival Significantly Faster than Departure */
	//int arrivalRate = 25;
	//int serviceRate = 5;
	// Result: good

	/* Test Case 2: Departure Significantly Slower than Arrival */
	/*int arrivalRate = 5;
	int serviceRate = 50;*/
	// Result: good

	double queueUtilization = arrivalRate / serviceRate;

	genArrivalEvents(arrivalValues, arrivalEvents, 
			EventType::Arrival, arrivalRate);
	genRandomValues(departValues, serviceRate);

	/* Function To Test */
	genDepartEvents(arrivalValues, departValues, departEvents);

	/* For the Finite Queue */
	genDepartEvents(arrivalValues, departValues, departEvents, queueCapacity);

	genObserverEvents(observeEvents, arrivalRate*6);

	const int totalSize = departEvents.size() + arrivalEvents.size() + observeEvents.size();
	array<EventOb, totalSize> allEvents;
	for (int i = 0; i < arrivalEvents.size(); i++) {
		allEvents[i] = arrivalEvents[i];
	}
	int baseIndex = arrivalEvents.size();
	for (int i = 0; i < departEvents.size(); i++) {
		allEvents[i + baseIndex] = departEvents[i];
	}
	baseIndex += departEvents.size();
	for (int i = 0; i < observeEvents.size(); i++) {
		allEvents[i + baseIndex] = observeEvents[i];
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


