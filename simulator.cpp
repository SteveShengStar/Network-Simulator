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
template<std::size_t DBL_ARRAY_SIZE, std::size_t EVENT_ARRAY_SIZE>
void genArrivalEvents(std::array<double, DBL_ARRAY_SIZE>& randValuesArray, std::array<EventOb, EVENT_ARRAY_SIZE>& randVarArray, EventType eType, int lambda) {

	double randVar;
	double sum = 0;

	for (int i = 0; i < randVarArray.size(); i++) { 
		randVarArray[i].id = i;
		randVarArray[i].eventType = eType;
	
		double temp = (float)rand() / (float)RAND_MAX;
		randVar = -log(1.00 - temp) / lambda;
		
		sum += randVar;
		randVarArray[i].instTime = sum;
		randValuesArray[i] = randVar;
	}
}

// Individual, non-accumulated values
template<std::size_t SIZE>
void genRandomValues(std::array<double, SIZE>& randomValArray, int lambda){
	for (int i = 0; i < randomValArray.size(); i++){
		double temp = (float)rand() / (float)RAND_MAX;
		double randVar = -log(1.00 - temp) / lambda;

		randomValArray[i] = randVar;
	}
}

template<std::size_t SIZE1, std::size_t SIZE2, std::size_t SIZE3>
void genDepartEvents(std::array<double, SIZE1>& arrivalValues, std::array<double, SIZE2>& departValues, std::array<EventOb, SIZE3>& departEvents) {

	std::queue<double> departQueue;
	int arrIndex = 0;
	int departIndex = 0;
	double elapsedTime = 0;

	departQueue.push(departValues[arrIndex]);
	elapsedTime += arrivalValues[arrIndex];
	arrIndex++;

	while (arrIndex < arrivalValues.size()){
		
		if (departQueue.empty() || (departQueue.front() > arrivalValues[arrIndex])) {
			double timePassed = arrivalValues[arrIndex];
			
			if (!departQueue.empty()) {
				// Does this work ?
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

	while(!departQueue.empty()) {
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

template<std::size_t SIZE>
void genObserverEvents(std::array<EventOb, SIZE>& observeEvents, int sampleRate) {
	double sum = 0.0;
	for (int i = 0; i < observeEvents.size(); i++){

		double temp = (float)rand() / (float)RAND_MAX;
		double randomTime = -log(1 - temp) / sampleRate;
		sum += randomTime;

		observeEvents[i].id = i;
		observeEvents[i].eventType = EventType::Observer;
		observeEvents[i].instTime = sum;
	}
}

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

int main() {
	const int SAMPLE_SIZE = 1000;
	std::array<double, SAMPLE_SIZE> arrivalValues;
	std::array<EventOb, SAMPLE_SIZE> arrivalEvents;
	std::array<double, SAMPLE_SIZE> departValues;
	std::array<EventOb, SAMPLE_SIZE> departEvents;
	std::array<EventOb, SAMPLE_SIZE*6> observeEvents;

	/* Normal Parameters */
	int serviceRate = 25;
	int arrivalRate = 75;

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


