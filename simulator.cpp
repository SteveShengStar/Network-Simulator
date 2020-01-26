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
		double temp = (float)randomTemp / (float)(RAND_MAX+1);
		randVar = -log(1.00 - temp) / lambda;

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
		double temp = (float)randomTemp / (float)(RAND_MAX+1);
		randVar = -log(1.00 - temp) / (float)lambda;
	} while (isinf(randVar));
	return randVar;
}


void genObserverEvents(vector<EventOb>& observeEvents, int sampleRate, const double totalSimTime) {
	double elapsedTime = 0.0;
	int eventId = 0;

	while (elapsedTime < totalSimTime) {
		int randomTemp = rand();
		double temp = (float)randomTemp / (float)(RAND_MAX+1);
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

	std::queue<double> waitQueue;
	vector<double>::iterator arrivalIterator = arrivalValues.begin();
	int eventIndex = 0;
	double elapsedTime = 0;

	while (elapsedTime < totalSimTime && arrivalIterator != arrivalValues.end()) {
		if (waitQueue.empty()) {
			elapsedTime += *arrivalIterator;

			if (elapsedTime >= totalSimTime) break;
			arrivalIterator++;

			waitQueue.push(genRandomValue(serviceRate, totalSimTime));
		}
		else if (waitQueue.front() < *arrivalIterator) {
			elapsedTime += waitQueue.front();
			if (elapsedTime >= totalSimTime) break;
			*arrivalIterator -= waitQueue.front();

			waitQueue.pop();
			
			// Create new Departure Event
			EventOb event;
			event.id = eventIndex;
			event.instTime = elapsedTime;
			event.eventType = EventType::Departure;
			event.packetDropped = false;
			departEvents.push_back(event);

			eventIndex++;
		}
		else if (waitQueue.front() > *arrivalIterator) {
			elapsedTime += *arrivalIterator;

			if (elapsedTime >= totalSimTime) break;
			waitQueue.front() -= *arrivalIterator;
			arrivalIterator++;

			waitQueue.push(genRandomValue(serviceRate, totalSimTime));
		}
		else {
			elapsedTime += *arrivalIterator;

			if (elapsedTime >= totalSimTime) break;
			arrivalIterator++;
			waitQueue.pop();
			waitQueue.push(genRandomValue(serviceRate, totalSimTime));

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
				// Transition from Idle to Active
				if (eventQueue.empty()) {
					queueState.idleTime += (event.instTime - lastTimeCheckpoint);
					queueState.stateLabel = QueueStateLabel::ACTIVE;
					lastTimeCheckpoint = event.instTime;
				}
				eventQueue.push(event);
				break;
			case EventType::Departure:
				if (eventQueue.empty()) { 
					cout << "Event Queue detected as empty" << endl;
					continue;
				}
				eventQueue.pop();

				// Transition from Active to Idle
				if (eventQueue.empty()) {
					queueState.activeTime += (event.instTime - lastTimeCheckpoint);
					queueState.stateLabel = QueueStateLabel::IDLE;
					lastTimeCheckpoint = event.instTime;
				}

				break;
			default:
				observations++;
				sumOfPacketsInQueueAllFrames += eventQueue.size();

				if (queueState.stateLabel == QueueStateLabel::IDLE) {
					queueState.idleTime += (event.instTime - lastTimeCheckpoint);
				} else {
					queueState.activeTime += (event.instTime - lastTimeCheckpoint);
				}
				lastTimeCheckpoint = event.instTime; 

				Statistics stat;
				stat.avgPacketsInQueue = (float)sumOfPacketsInQueueAllFrames / (float)observations;
				stat.idleTime = queueState.idleTime / event.instTime;
				stat.lossRatio = -1.0;
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

	srand(time(0));

	std::vector<double> arrivalValues;
	std::vector<EventOb> arrivalEvents;
	std::vector<EventOb> departEvents;
	std::vector<EventOb> observeEvents;

	/* Normal Parameters */
	const int SERVICE_RATE = 1000000;
	const double packLength = 2000.0;
	const int TOTAL_SIMTIME = 1000; 
	const int QUEUE_CAPACITY = 10;

	ofstream myfile;
	myfile.open("data/output5.csv");
	myfile << "Row Constant Value, Av. No. Packets in Buffer, Idle Time %" << endl;
	
	for (double ROW_CONST = 0.25; ROW_CONST < 1; ROW_CONST += 0.1) {
		double ARRIVAL_RATE = SERVICE_RATE * ROW_CONST / packLength;

		genArrivalEvents(arrivalValues, arrivalEvents,
			EventType::Arrival, ARRIVAL_RATE, TOTAL_SIMTIME);

		/* For Infinite Queue */
		double lambdaForDepartEvents = SERVICE_RATE / packLength;
		genDepartEvents(arrivalValues, lambdaForDepartEvents, departEvents, TOTAL_SIMTIME);

		/* For the Finite Queue */
		//genDepartEvents(arrivalValues, arrivalEvents, lambdaForDepartEvents, departEvents, TOTAL_SIMTIME, QUEUE_CAPACITY);

		genObserverEvents(observeEvents, ARRIVAL_RATE * 6, TOTAL_SIMTIME);

		// Clean up the data
		vector<EventOb>::iterator arrivalIt = arrivalEvents.begin();
		vector<EventOb>::iterator departIt = departEvents.begin();
		vector<int> problemIndices;
		int itIndex = 0;
		while (arrivalIt != arrivalEvents.end() && departIt != departEvents.end()) {
			if (arrivalIt->instTime > departIt->instTime) {
				int problemIndex = itIndex - problemIndices.size();
				problemIndices.push_back(problemIndex);
			}
			arrivalIt++;
			departIt++;
			itIndex++;
		}
		for (vector<int>::iterator it = problemIndices.begin(); it != problemIndices.end(); it++) {
			departEvents.erase(departEvents.begin() + *it);
			arrivalEvents.erase(arrivalEvents.begin() + *it);
		}
		problemIndices.clear();

		// Put all events into one vector
		vector<EventOb> allEvents(arrivalEvents);
		vector<EventOb>::iterator evIterator = allEvents.end();
		allEvents.insert(evIterator, departEvents.begin(), departEvents.end());
		evIterator = allEvents.end();
		allEvents.insert(evIterator, observeEvents.begin(), observeEvents.end());
		

		// Sort Events based on simulation time
		sort(allEvents.begin(), allEvents.end(), compare);
		// For events occurring at the same simulation time, make sure Departure Event is ordered last
		vector<EventOb>::iterator prev = allEvents.begin();
 		vector<EventOb>::iterator current = allEvents.begin();
		current++;
		while (current != allEvents.end()) {
			if (prev->instTime == current->instTime) {
				if (prev->eventType == EventType::Departure) {
					swap(*prev, *current);
				}
			}
			prev++;
			current++;
		}

		// DEBUGGING: Test that number of departed packets never exceeds number of arrived packets
		int arrivalCount = 0;
		int departCount = 0;
		int amountOfErrorPoints = 0;
		prev = allEvents.begin();
		current = allEvents.begin();
		switch (current->eventType) {
		case EventType::Arrival:
			arrivalCount++;
			break;
		case EventType::Departure:
			departCount++;
			break;
		}
		current++;
		while (current != allEvents.end()) {
			switch (current->eventType) {
				case EventType::Arrival:
					arrivalCount++;
					break;
				case EventType::Departure:
					departCount++;
					break;
			}
			if (departCount > arrivalCount) {
				amountOfErrorPoints++;
			}
			current++;
			prev++;
		}
		myfile << amountOfErrorPoints << endl;


		// Print statistics for MM1 Queue
		vector<Statistics> stats = runDESimulator(allEvents);
		double idleRatio = 0.0;
		double averagePacketsInQueue = 0.0;
		vector<Statistics>::iterator itStats = stats.end();
		itStats--;
		const int SAMPLES_COLLECTED = 4;
		for (int i = 0; i < SAMPLES_COLLECTED; i++, itStats--) {
			idleRatio += itStats->idleTime;
			averagePacketsInQueue += itStats->avgPacketsInQueue;
		}


		myfile << ROW_CONST << ",";
		myfile << (averagePacketsInQueue / (float)SAMPLES_COLLECTED) << ",";
		myfile << (idleRatio / (float)SAMPLES_COLLECTED) << endl;

		arrivalValues.clear();
		arrivalEvents.clear();
		departEvents.clear();
		observeEvents.clear();
		stats.clear();
		allEvents.clear();
	}
	myfile.close();

	return 0;
}