#include <MainHeaders.h>

class Calculation
{

public:

	static int NumberOfGeneratedConnections;															//Keeps the number of generated connections during each run of simulation.
	static std::vector<double> LoadProfile;																//Keeps the profile of traffic in terms of number of requested frequency slots.
	static int PathProfile[50];																			//Keeps the path profile.
	static double AverageBitRatePerConnection;															//Keeps the average bit rate per connection. It means get an average form load profile elements.
	static double NumberofDrops;																		//Keeps the  drops in Gbps.
	static double NumberofReceived;																		//Keeps the receives in Gbps.
	static double ReceivedSpectrum;																		//Keeps the receives in GHz.
	static std::vector<double> BlockingProbabilityPerRun;												//Blocking probability per run.
	static std::vector<double> SpectrumUtilizationPerRun;												//Spectrum utilization per run.
	static std::vector<double> AverageNumberOfGeneratedConnections;										//An average number that shows the number of generated connections per run.
	static std::vector<double> AverageDrop;																//An average number that shows the data drop per run.
	static std::vector<double> AverageReceive;															//An average number that shows the data received per run.
	static std::vector<double> AverageReceiveSpectrum;													//An average spectrum that shows the spectrum of data received per run.
	static std::vector<double> AverageGuardBandPercentage;												//An average number that shows percentage of guard bands respecting to the total slots in the network.
	static std::vector<double> AverageSpectrumUsagePercentage;											//An average number that shows percentage of used slots respecting to the total slots in the network.
	static std::vector<double> AverageFragmentationLevel;

	void GeneratedConnectionIncrement() {NumberOfGeneratedConnections++;};								//Increments the number of generated connections.
	void GeneratedConnectiondisplay();																	//Shows the number of generated connections.
	void LoadProfileIncrement(double bandwidth) {this->LoadProfile.push_back(bandwidth);};				//Increase the appropriated profile array.
	void PathProfileIncrement(int pathlenght) {PathProfile[pathlenght]++;};
	void PathProfiledisplay();
	void CalculateAverageBitRatePerConnection();														//Calculates the average bit rate per connection.
	void ShowAverageBitRatePerConnection();																//Shows the average bit rate per connection.
	void NumberOfDropsIncrement(double Dropsize) {NumberofDrops+=(Dropsize);};							//The amount of drooped data (Gbps).
	void ShowNumberofDrops();																			//Shows the number of drops per run.
	void NumberOfReceivesIncrement(double Receivesize) {NumberofReceived+=(Receivesize);};				//The amount of received data (Gbps).
	void ReceivesSpectrumIncrement(double ReceiveSpectrum) {ReceivedSpectrum+=(ReceiveSpectrum);};		//The amount of received data (Gbps).
	void ShowNumberofReceives();																		//Shows the number of receives per run.
	void ShowSpectrumofReceives();																		//Shows the spectrum of receives per run.
	void BlockingProbabilityCalculation();																//Blocking probability per run calculation.
	void GuardBandWasteCalculation(double TotalGuardBands, double TotalSlotsInNetwork);					//Guard band calculations.
	void SpectrumUsageCalculation(double TotalUsedSlots, double TotalSlotsInNetwork);					//Spectrum usage calculations.
	bool SimulationEnding();																			//Check the simulation ending using confidence interval.
	std::vector<double> Finaldisplay();																	//Prepare the final data and displays them.

};

int Calculation::NumberOfGeneratedConnections = 0;
std::vector<double> Calculation::LoadProfile;
int Calculation::PathProfile[50] = {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};
double Calculation::AverageBitRatePerConnection = 0;
double Calculation::NumberofDrops = 0;
double Calculation::NumberofReceived = 0;
double Calculation::ReceivedSpectrum = 0;
std::vector<double> Calculation::BlockingProbabilityPerRun;
std::vector<double> Calculation::SpectrumUtilizationPerRun;
std::vector<double> Calculation::AverageNumberOfGeneratedConnections;
std::vector<double> Calculation::AverageDrop;
std::vector<double>	Calculation::AverageReceive;
std::vector<double>	Calculation::AverageReceiveSpectrum;
std::vector<double> Calculation::AverageGuardBandPercentage;
std::vector<double> Calculation::AverageSpectrumUsagePercentage;
std::vector<double> Calculation::AverageFragmentationLevel;

void Calculation::GeneratedConnectiondisplay() {EV <<"Number of Generated Traffic so far : \t" << this->NumberOfGeneratedConnections << endl;}

void Calculation::PathProfiledisplay()
{
	EV <<"Path Profile : \t \t \t";
	for(int *p = this->PathProfile+1; p < this->PathProfile+50; p++)
		EV << *p << " ";
	EV << "\n" << endl;
}

void Calculation::CalculateAverageBitRatePerConnection()
{
	this->AverageBitRatePerConnection = 0;

	for(unsigned int p = 0; p < this->LoadProfile.size(); p++)
		this->AverageBitRatePerConnection = this->AverageBitRatePerConnection + this->LoadProfile.at(p);

	this->AverageBitRatePerConnection = this->AverageBitRatePerConnection/((double) this->NumberOfGeneratedConnections);
}

void Calculation::ShowAverageBitRatePerConnection() {EV <<"Average bit rate per connection : \t" << this->AverageBitRatePerConnection << "  (GHz) \n" << endl;}

void Calculation::ShowNumberofDrops() {EV <<"Dropped data : \t" << this->NumberofDrops << "  Gbps \n" << endl;}

void Calculation::ShowNumberofReceives() {EV <<"Received data : \t" << this->NumberofReceived << "  Gbps" << endl;}

void Calculation::ShowSpectrumofReceives() {EV <<"Received data spectrum : \t" << this->ReceivedSpectrum << "  GHz \n" << endl;}

void Calculation::BlockingProbabilityCalculation()
{
	//Save some parameter as an extra information.
	this->AverageNumberOfGeneratedConnections.push_back(this->NumberOfGeneratedConnections);
	this->AverageDrop.push_back(this->NumberofDrops);
	this->AverageReceive.push_back(this->NumberofReceived);
	this->AverageReceiveSpectrum.push_back(this->ReceivedSpectrum);

	//Blocking probability calculation.
	double Dropped = this->NumberofDrops;
	double Received = this->NumberofReceived;

	//Spectrum utilization calculation.
	double UsedSpectrum = this->ReceivedSpectrum;

	double BlockingProbability = Dropped/(Dropped+Received);			//BP = Dropped/(Dropped+Received).
	double SpectrumUtilization = Received/UsedSpectrum;                 //Spectrum utilization (bite/s/Hz).

	this->BlockingProbabilityPerRun.push_back(BlockingProbability);		//Save BP in a vector. The final BP is mean of values in this vector.
	this->SpectrumUtilizationPerRun.push_back(SpectrumUtilization);     //Save Spe_Util in a vector. The final Spe_Util is mean of values in this vector.

	//After each run, all parameters have to be zero again.
	this->NumberofDrops = 0;
	this->NumberofReceived = 0;
	this->ReceivedSpectrum = 0;
	this->NumberOfGeneratedConnections = 0;
}

void Calculation::GuardBandWasteCalculation(double TotalGuardBands, double TotalSlotsInNetwork)
{
	this->AverageGuardBandPercentage.push_back(TotalGuardBands/TotalSlotsInNetwork);

	EV <<"Average Guard Band Percentage : " << AverageGuardBandPercentage.at(0) << endl;
}

void Calculation::SpectrumUsageCalculation(double TotalUsedSlots, double TotalSlotsInNetwork)
{
	this->AverageSpectrumUsagePercentage.push_back(TotalUsedSlots/TotalSlotsInNetwork);

	EV <<"Average Spectrum Usage Percentage : " << AverageSpectrumUsagePercentage.at(0) << endl;
}

bool Calculation::SimulationEnding()
{
	bool SimulationEnding = false;

	//Mean of collected blocking probabilities.
	double mean = 0;
	for(unsigned i=0; i < this->BlockingProbabilityPerRun.size(); i++)
		mean = mean + this->BlockingProbabilityPerRun.at(i);

	mean = mean/this->BlockingProbabilityPerRun.size();

	//In order to calculate the confidence interval, first squared difference has to be calculated.
	std::vector <double> SquaredDifference;
	SquaredDifference.assign(this->BlockingProbabilityPerRun.size(),0);
	for(unsigned i=0; i < this->BlockingProbabilityPerRun.size(); i++)
		SquaredDifference.at(i) = (this->BlockingProbabilityPerRun.at(i)-mean)*(this->BlockingProbabilityPerRun.at(i)-mean);

	//Now, Variance.
	double variance = 0;
	for(unsigned int i=0; i < this->BlockingProbabilityPerRun.size(); i++)
		variance = variance + SquaredDifference.at(i);

	variance = variance/this->BlockingProbabilityPerRun.size();

	//Confidence Interval.
	if(((1.96*variance)/sqrt(this->BlockingProbabilityPerRun.size())) <= mean/10)  SimulationEnding = true;

	return SimulationEnding;
}

std::vector<double> Calculation::Finaldisplay()
{
	//Calculates the average values for saved data. These values will be displayed in the final result .txt file.
	std::vector<double> DataReadyForDisplay;

	double mean_BP = 0;
	double mean_SU = 0;
	double mean_Drop = 0;
	double mean_Sent = 0;
	double mean_Received = 0;
	double mean_ReceivedSpectrum = 0;
	unsigned int TotalNumberOfRuns = this->BlockingProbabilityPerRun.size();

	for(unsigned i=0; i<TotalNumberOfRuns; i++)
	{
		mean_Drop = mean_Drop + (this->AverageDrop.at(i)/TotalNumberOfRuns);
		mean_Received = mean_Received + (this->AverageReceive.at(i)/TotalNumberOfRuns);
		mean_ReceivedSpectrum = mean_ReceivedSpectrum + (this->AverageReceiveSpectrum.at(i)/TotalNumberOfRuns);
		mean_BP = mean_BP + (this->BlockingProbabilityPerRun.at(i)/TotalNumberOfRuns);
		mean_SU = mean_SU + (this->SpectrumUtilizationPerRun.at(i)/TotalNumberOfRuns);
		mean_Sent = mean_Sent + (this->AverageNumberOfGeneratedConnections.at(i)/TotalNumberOfRuns);
	}

	DataReadyForDisplay.push_back(mean_BP);
	DataReadyForDisplay.push_back(mean_Sent);
	DataReadyForDisplay.push_back(mean_Drop);
	DataReadyForDisplay.push_back(mean_Received);
	DataReadyForDisplay.push_back(mean_ReceivedSpectrum);
	DataReadyForDisplay.push_back(mean_SU);

	return DataReadyForDisplay;
}
