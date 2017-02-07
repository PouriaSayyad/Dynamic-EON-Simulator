#include <Calculations.h>

struct Slice{std::vector<long> FrequencySlots;};																//An array which represents available frequency slots per link.

struct ExistingConnectionsInfo																					//The information of existing connections in the network.
{
	long ID; int Source;int Bandwidth;int Destination;int Transmitter; int GuardBand;
	std::vector <int> path;
};

class Spectrum
{
	public:

		static std::vector<Slice> links;																												//An array which represents links in the network. See description below.
		static std::vector<ExistingConnectionsInfo> ExistingConnectionsList;																			//An array that keeps the information of existing connections in the network.

		void BuildingSpectrumModel(double FrequencySlotPerLink, int NumberOfNodes, double NumberOfCoresModes);											//Building the network's spectrum model.
		void ShowSpectrumModel(double FrequencySlotPerLink, int NumberOfNodes, double NumberOfCoresModes);												//Show the network's spectrum model.
		std::vector<long> ContinuityConstraint (double FrequencySlotPerLink, int NumberOfNodes, std::vector<int> path);									//Continuity Constraint.
		void ShowSpectrumStatus(std::vector<long> SpectrumStatus, double NumberOfCoresModes);																						//Show spectrum status.
		std::vector<int> ContiguityConstraint(std::vector<long> SpectrumStatus, Request *msg, int RequiredSpectrum, double NumberOfCoresModes);			//Contiguity Constraint.
		void ShowSpectrumRange(std::vector<int> SpectrumRange);																							//Show allocated spectrum range.
		bool Allocation(std::vector<int> SpectrumRange, Request *msg, std::vector<int> path, int NumberOfNodes, double TotalResourcesPerLink);										//Allocation.
		void ConnectionTermination(Request *msg, int NumberOfNodes, double TotalResourcesPerLink);														//Connection termination function.
		void SaveOnTheExistingConnectionsList(Request *msg, std::vector <int> path);																	//This function saves the information of existing connections in the network.
		void RemoveFromTheExistingConnectionsList(Request *msg);																						//This function removes the information from the existing connections.
		std::vector<int> Space_ContiguityConstraint(std::vector<long> SpectrumStatus, Request *msg, int RequiredSpectrum, double NumberOfCoresModes);	//Contiguity Constraint in Space.
		void Space_ShowSpectrumRange(std::vector<int> SpectrumRange, double FrequencySlotPerLink);														//Show allocated spectrum range.
		bool Space_Allocation(std::vector<int> SpectrumRange, Request *msg, std::vector<int> path, int NumberOfNodes, double TotalResourcesPerLink);	//Allocation.
};

std::vector<Slice> Spectrum::links;
std::vector<ExistingConnectionsInfo> Spectrum::ExistingConnectionsList;

void Spectrum::BuildingSpectrumModel(double FrequencySlotPerLink, int NumberOfNodes, double NumberOfCoresModes)
{
	Slice ini;
	ini.FrequencySlots.assign(NumberOfCoresModes*FrequencySlotPerLink, 0);
	this->links.assign(NumberOfNodes*NumberOfNodes, ini);
}

void Spectrum::ShowSpectrumModel(double FrequencySlotPerLink, int NumberOfNodes, double NumberOfCoresModes)
{
	EV <<"The spectrum model of network \n _______________________________________ \n" <<endl;
	EV <<"S    D   C/M    Frequency Slots\n" <<endl;
	for(int i=0; i<NumberOfNodes; i++)
		for(int j=0; j<NumberOfNodes; j++)
		{
			for(int m=0; m<NumberOfCoresModes; m++)
			{
				EV << i << "    " << j <<"    "<< m <<"    ";
				for(int z=0; z<FrequencySlotPerLink; z++)
					EV <<this->links.at(i*NumberOfNodes+j).FrequencySlots.at(m*FrequencySlotPerLink+z) << " ";
				EV <<"\n" << endl;
			}
			EV <<"-------------------------\n" << endl;
		}
}

std::vector<long> Spectrum::ContinuityConstraint(double TotalResourcesPerLink,int NumberOfNodes, std::vector<int> path)
{
	//Continuity Constraint: add all the spectrum of links along the path and retrun the result.
	std::vector<long> SpectrumStatus;
	SpectrumStatus.assign(TotalResourcesPerLink, 0);

	int ArrayIndex;					//The index of link. this->links.at(ArrayIndex).FrequencySlots.at(j) : slot j at link ArrayIndex.
	for(unsigned int i=0; i<path.size()-1; i++)
	{
		ArrayIndex = (path.at(i)* NumberOfNodes) + path.at(i+1);
		for(int j=0; j<TotalResourcesPerLink; j++)
			SpectrumStatus.at(j) = SpectrumStatus.at(j) + this->links.at(ArrayIndex).FrequencySlots.at(j);
	}

	return SpectrumStatus;
}

void Spectrum::ShowSpectrumStatus(std::vector<long> SpectrumStatus, double NumberOfCoresModes)
{
	//Show spectrum status.
	double FrequencySlotPerLink = (SpectrumStatus.size()/NumberOfCoresModes);
	EV <<"Spectrum Status :\n" << endl;
	for(unsigned int j = 0; j < NumberOfCoresModes; j++)
	{
		EV <<"C/M " << j << "  : \t";
		for(unsigned int i = 0; i <FrequencySlotPerLink; i++)
			EV << SpectrumStatus.at(j*FrequencySlotPerLink+i) << " ";
		EV <<"\n" << endl;
	}
}

std::vector<int> Spectrum::ContiguityConstraint(std::vector<long> SpectrumStatus, Request *msg, int RequiredSpectrum, double NumberOfCoresModes)
{
	//Contiguity Constraint.
	std::vector<int> SpectrumRange;									//This array shows the starting and ending allocated frequency slots.

	int End = 0;
	int Start = 0;
	int Core_Mode = 10000;

	double FrequencySlotPerLink = (SpectrumStatus.size()/NumberOfCoresModes);

	for(unsigned int CM=0; CM<NumberOfCoresModes; CM++)
	{
		for(unsigned int i=0; i<FrequencySlotPerLink; i++)
		{
			if(SpectrumStatus.at(CM*FrequencySlotPerLink+i) == 0)
			{
				Start = i;
				do
				{
					End++;
					i++;

				}while(i<FrequencySlotPerLink && End != RequiredSpectrum && SpectrumStatus.at(CM*FrequencySlotPerLink+i) == 0);

				if(End == RequiredSpectrum)
				{
					Core_Mode = CM;
					End = Start + End;
					break;
				}else End = Start = 0;
			}
		}

		if(Core_Mode != 10000)
			break;
	}

	SpectrumRange.push_back(Start);
	SpectrumRange.push_back(End);
	SpectrumRange.push_back(Core_Mode);

	return SpectrumRange;
}

void Spectrum::ShowSpectrumRange(std::vector<int> SpectrumRange)
{
	EV <<"The first available spectrum is from  " << SpectrumRange.at(0) << "  to   " << SpectrumRange.at(1) << "  Over Core/Mode   " << SpectrumRange.at(2) << "\n" << endl;
}

bool Spectrum::Allocation(std::vector<int> SpectrumRange, Request *msg, std::vector<int> path, int NumberOfNodes, double FrequencySlotPerLink)
{
	//This function will allocate connections in the network.
	int ArrayIndex;
	bool NotAllocated = false;

	for(unsigned int i=0; i<path.size()-1; i++)
	{
		ArrayIndex = (path.at(i)* NumberOfNodes) + path.at(i+1);
		for(int j=SpectrumRange.at(0); j<SpectrumRange.at(1); j++)
			this->links.at(ArrayIndex).FrequencySlots.at(SpectrumRange.at(2)*FrequencySlotPerLink+j)= msg->getId();

		//In case of bi-directional connection establishment.
		/*	ArrayIndex = (path.at(i+1)* NumberOfNodes) + path.at(i);
			for(int j=SpectrumRange.at(0); j<SpectrumRange.at(1); j++)
				this->links.at(ArrayIndex).FrequencySlots.at(SpectrumRange.at(2)*FrequencySlotPerLink+j);*/
	}

	return NotAllocated;
}

void Spectrum::ConnectionTermination(Request *msg, int NumberOfNodes, double TotalResourcesPerLink)
{
	//A search on all elements of array to find terminating connection Id.
	for(int i=0; i<(NumberOfNodes * NumberOfNodes); i++)
		for(int j=0; j<TotalResourcesPerLink; j++)
				if(this->links.at(i).FrequencySlots.at(j) == msg->getId())
					this->links.at(i).FrequencySlots.at(j) = 0;
}

void Spectrum::SaveOnTheExistingConnectionsList(Request *msg, std::vector <int> path)
{
	//It is activated after successful establishment of a connection over the network.
	EV <<"Number of saved data on the existing connections list before add : \t" << ExistingConnectionsList.size() << endl;
	ExistingConnectionsInfo SavableInfo;

	SavableInfo.ID = msg->getId();
	SavableInfo.Source = msg->getSource();
	SavableInfo.Bandwidth = msg->getBandwidth();
	SavableInfo.Destination = msg->getDestination();

	for(unsigned int i=0; i<path.size(); i++)
		SavableInfo.path.push_back(path.at(i));

	//Just to check that weather the saved path is right or not
	/*EV <<" " << endl;
	for(unsigned int i=0; i<path.size(); i++)
		EV << SavableInfo.path.at(i) << " ";
	EV <<"\n" <<endl;*/

	ExistingConnectionsList.push_back(SavableInfo);

	EV <<"Number of saved data on the existing connections list after add : \t" << ExistingConnectionsList.size() << endl;
}

void Spectrum::RemoveFromTheExistingConnectionsList(Request *msg)
{
	//It is activated after termination of a connection.

	EV <<"Number of saved data on the existing connections list before erase : \t" << ExistingConnectionsList.size() << endl;

	for(unsigned int i=0; i<ExistingConnectionsList.size(); i++)
		if(msg->getId() == ExistingConnectionsList.at(i).ID)
			ExistingConnectionsList.erase(ExistingConnectionsList.begin()+i);

	EV <<"Number of saved data on the existing connections list after erase : \t" << ExistingConnectionsList.size() << "\n" << endl;
}

std::vector<int> Spectrum::Space_ContiguityConstraint(std::vector<long> SpectrumStatus, Request *msg, int RequiredSpectrum, double NumberOfCoresModes)
{
	//Space Contiguity Constraint.
	std::vector<int> SpectrumRange;									//This array shows the starting and ending allocated frequency slots.

	int End = 0;
	int Start = 0;
	int Slot_Index = 10000;

	double FrequencySlotPerLink = (SpectrumStatus.size()/NumberOfCoresModes);

	for(unsigned int i=0; i<FrequencySlotPerLink; i++)
	{
		for(unsigned int CM=0; CM<NumberOfCoresModes; CM++)
		{
			if(SpectrumStatus.at(CM*FrequencySlotPerLink+i) == 0)
			{
				Start = CM;
				do
				{
					End++;
					CM++;

				}while(CM<NumberOfCoresModes && End != RequiredSpectrum && SpectrumStatus.at(CM*FrequencySlotPerLink+i) == 0);

				if(End == RequiredSpectrum)
				{
					Slot_Index = i;
					End = Start + End;
					break;
				}else End = Start = 0;
			}
		}

		if(Slot_Index != 10000)
			break;
	}

	SpectrumRange.push_back(Start);
	SpectrumRange.push_back(End);
	SpectrumRange.push_back(Slot_Index);

	return SpectrumRange;
}

void Spectrum::Space_ShowSpectrumRange(std::vector<int> SpectrumRange, double FrequencySlotPerLink)
{
	EV <<"The first available Core/Mode is  " << SpectrumRange.at(0) << "  to   " << SpectrumRange.at(1) << "  Over Slot index   " << SpectrumRange.at(2) << "\n" << endl;
}

bool Spectrum::Space_Allocation(std::vector<int> SpectrumRange, Request *msg, std::vector<int> path, int NumberOfNodes, double FrequencySlotPerLink)
{
	//This function will allocate connections in the network.
	int ArrayIndex;
	bool NotAllocated = false;

	for(unsigned int i=0; i<path.size()-1; i++)
	{
		ArrayIndex = (path.at(i)* NumberOfNodes) + path.at(i+1);
		for(int j=SpectrumRange.at(0); j<SpectrumRange.at(1); j++)
			this->links.at(ArrayIndex).FrequencySlots.at(j*FrequencySlotPerLink+SpectrumRange.at(2))= msg->getId();

		//In case of bi-directional connection establishment.
		/*	ArrayIndex = (path.at(i+1)* NumberOfNodes) + path.at(i);
			for(int j=SpectrumRange.at(0); j<SpectrumRange.at(1); j++)
				this->links.at(ArrayIndex).FrequencySlots.at(j*FrequencySlotPerLink+SpectrumRange.at(2));*/
	}

	return NotAllocated;
}


/* Two Dimensions, One Dimension

Let's say you have a two dimensional array: MyArray(9,9).
That's an array with 100 elements; starting with MyArray(0,0), all the way to MyArray(9,9) and everything in between.
Suppose you wanted to treat the elements of this array as a one dimensional array.
In other words, you wanted to think of the array like this:

Element  #0: MyArray(0,0)
Element  #1: MyArray(0,1)
Element  #2: MyArray(0,2)
...
Element  #9: MyArray(0,9)
Element #10: MyArray(1,0)
Element #11: MyArray(1,1)
...
Element #97: MyArray(9,7)
Element #98: MyArray(9,8)
Element #98: MyArray(9,9)

This is a situation you may run into from time to time, and it's good to be able to "walk through" a two dimensional array,
assigning a one-dimensional index to each element. Now, for this example, I've picked something easy, because you'll notice
that the ones and tens places of the one-dimensional index correspond to the X and Y dimensions of the two-dimensional indices.
What's the one-dimensional index for MyArray(7,8)? It's easy! Put the 7 and the 8 together, and you've got Element #78.

But we need to express that mathematically, so the computer can understand it.

What is the "tens" place of a number? The tens place tells you "how many tens" are in a number.
For example, the number 78 is "seven tens and eight more", or 7 x 10 + 8. Great! That tells us
how to create our one-dimensional index! Check this out:

Dim MyArray(9,9)
Dim X As Integer
Dim Y As Integer

'Get the one-dimensional index
Public Function ArrayIndex(Y As Integer, X As Integer)
   ArrayIndex = (Y * 10) + X
End Function

In our code: ArrayIndex = (Source node * Number of nodes in the network) + Destination node
 */

