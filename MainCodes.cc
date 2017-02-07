#include <SpectrumModel.h>

class Router : public cSimpleModule, public Calculation, public Spectrum
{
	int NumberOfRuns;										//Number of runs.
	int RequestsPerRun;										//Generated connections per run.

	cMessage *Send_event;	 								//Traffic generation trigger in accordance with the Poisson distribution.
	Calculation *InformationCollector;						//Collecting the number of sent, received and dropped connections per run.
	Spectrum *SpectrumResourcesPerLink;						//Spectrum resources per link.

	std::vector <int> path;	 								//Kth shortest path.

	void initialize();
	void handleMessage(cMessage *msg);
	Request * generateMessage ();
	Request * ModulationLevelSeclection(Request *msg, std::vector <int> CurrentPath);
	std::vector <int> PhatFinder(Request *msg, cTopology *topo);
	void DisplayPath (std::vector <int> path, Request *msg);
	bool UpdateTopology(std::vector <int> path, cTopology *topo, Request *msg);
	void ConnectionDrop(Request *msg);
	void ConnectionReceived(Request *msg);
	double SubBandGB(double SubBandResolution, double SubBandXtalk);
	void finish();
};

Define_Module(Router);

void Router::initialize()
{
	//Set simulation values. Number of runs and number of connection requests per run.
	NumberOfRuns = par("_NumberOfRuns");
	RequestsPerRun = par("_RequestsPerRun");

	//Build the spectrum model parameters.
	if(getIndex()==0)			//Since it has been introduced as an static parameter, so the initialization happen only for the first node of network.
	{
		cTopology *topo = new cTopology("topo");
		topo->extractByNedTypeName(cStringTokenizer("Router").asVector());
		SpectrumResourcesPerLink->BuildingSpectrumModel(par("_FrequencySlotPerLink"), topo->getNumNodes(), par("_NumberOfCoresModes"));
		//SpectrumResourcesPerLink->ShowSpectrumModel(par("_FrequencySlotPerLink"), topo->getNumNodes(), par("_NumberOfCoresModes"));
		delete topo;
	}

	//Initialising the traffic generation based on the Poisson distribution. If is only for TID topology.
	if(getIndex()==3 || getIndex()==4 || getIndex()==28 || getIndex()==27 || getIndex()==22 || getIndex()==23 || getIndex()==7 || getIndex()==8 || getIndex()==11 || getIndex()==10 || getIndex()==18 || getIndex()==16 || getIndex()==14 || getIndex()==13)
	{
		Send_event = new cMessage("Send_event");
		Send_event->setKind(0);
		scheduleAt(par("_IAT"), Send_event);
	}
}

void Router::handleMessage(cMessage *msg)
{
	if(msg->getKind() == 0)
	{
		//Connection establishment: it consists solving the problem of routing and spectrum allocation.
		//New connection request generation.
		Request *res = generateMessage ();													//Generates a new connection request.

		//Some information collection. Number of generated connections, load profile and average bit rate per connection.
		InformationCollector->GeneratedConnectionIncrement();								//Increments the number of generated connections.
		InformationCollector->GeneratedConnectiondisplay();									//Shows the number of generated connections.

		InformationCollector->LoadProfileIncrement(res->getRealTrafficDemand());			//Increase traffic.
		if(InformationCollector->BlockingProbabilityPerRun.empty())
		{
			InformationCollector->CalculateAverageBitRatePerConnection();					//Calculates the average bit rate per connection.
			InformationCollector->ShowAverageBitRatePerConnection();						//Shows the average bit rate per connection.
		}

		//Solving the RSA problem.
		//Routing using Kth disjoint shortest paths.
		//Spectrum allocation using First Fit.

		path.clear();
		int K = 0;																			//Kth shortest path.
		bool ThereIsNoRoute = false;														//Boolean variable to show there is a route to accommodate the connection.
		bool NotAllocated = true;															//Shows that the allocation has been not occurred yet. If true next shortest path has to be checked.

		cTopology *topo = new cTopology("topo");											//Topology extraction from .ned fine.
		topo->extractByNedTypeName(cStringTokenizer("Router").asVector());

		cTopology::Node *SourceNode1 = topo->getNode(res->getSource());
		cTopology::Node *DestinationNode1 = topo->getNode(res->getDestination());
		int NodalDegreeSource = SourceNode1->getNumOutLinks();
		int NodalDegreeDestination = DestinationNode1->getNumOutLinks();

		int K_limit = NodalDegreeSource;													//K can be limited by three factors: 1) nodal degree of the source node, 2) nodal degree of the destination node and 3) K = 3.
		if(K_limit > NodalDegreeDestination)  K_limit = NodalDegreeDestination;
		if(K_limit > 3)	K_limit = 3;

		EV <<"K limit is: \t" << K_limit <<endl;

		while(NotAllocated)
		{
			//Routing sub problem.
			K++;
			if(K > K_limit) {ConnectionDrop(res); break;}									//3 shortest path.
			else if(K > 1) ThereIsNoRoute = UpdateTopology(path, topo, res);				//Update topology for next shortest path calculation if necessary.

			if(ThereIsNoRoute)	{EV<<"There is no route.\n"<<endl;ConnectionDrop(res); break;}

			path.clear();																	//To start solving, an empty path array is needed.
			path = PhatFinder(res, topo);													//Kth shortest path calculator.
			DisplayPath(path, res);															//Kth shortest path display.

			//Modulation level selection.
			res = ModulationLevelSeclection(res, path);
			if (res->getBandwidth() == 0) continue;

			//Spectrum allocation sub problem.
			//Continuity constraint.
			double NumberOfCoresModes = par("_NumberOfCoresModes");
			double FrequencySlotPerLink = par("_FrequencySlotPerLink");

			int Scenario = par("_Scenario");
			if(Scenario == 1)
			{
				int BreakDown;
				std::vector<int> Breaked;
				EV <<"Bandwidth \t" << res->getBandwidth() << endl;

				bool Request_BreakDown_On = par("_Request_BreakDown_On");
				int Max_SpChTRx_Capacity = par("_Max_SpChTRx_Capacity");
				//This is valid if we consider the type of TRx that we discussed with Ioannis.
				if(Request_BreakDown_On){
					BreakDown = res->getBandwidth();
					if (BreakDown > Max_SpChTRx_Capacity){
						int Temp = res->getBandwidth();
						while (Temp > Max_SpChTRx_Capacity){
							Breaked.push_back(Max_SpChTRx_Capacity);
							Temp = Temp - Max_SpChTRx_Capacity;}
						if (Temp > 0)
							Breaked.push_back(Temp);
					}else{
						Breaked.push_back(res->getBandwidth());}

					EV <<"Break Down : \t" << BreakDown << endl;
					EV <<"Sequence: \t";
					for (unsigned int i=0; i<Breaked.size(); i++)
						EV << Breaked.at(i) << " ";
					EV <<"\n\n" << endl;
				}else{
					Breaked.push_back(res->getBandwidth());}

				int ServedSoFarBP = 0;
				double ServedSoFarTD = 0;
				for (unsigned int i=0; i<Breaked.size(); i++)
				{
					//Spectrum Contiguity Constraint.
					std::vector<long> SpectrumStatus = SpectrumResourcesPerLink->ContinuityConstraint(NumberOfCoresModes*FrequencySlotPerLink, topo->getNumNodes(), path);
					SpectrumResourcesPerLink->ShowSpectrumStatus(SpectrumStatus, NumberOfCoresModes);//Show spectrum status.

					Request *DupRes = res->dup();
					DupRes->setBandwidth(Breaked.at(i));
					DupRes->setRealTrafficDemand((double) Breaked.at(i)*((double) res->getRealTrafficDemand()/res->getBandwidth()));

					std::vector<int> SpectrumRange = SpectrumResourcesPerLink->ContiguityConstraint(SpectrumStatus, DupRes, DupRes->getBandwidth(), NumberOfCoresModes);
					SpectrumResourcesPerLink->ShowSpectrumRange(SpectrumRange);
					if(SpectrumRange.at(0) != SpectrumRange.at(1))								//if the beginning and ending of the range are not equal means it is possible to allocate connection.
					{
						//Connection allocation. NotAllocated = False if the connection allocate.
						NotAllocated = SpectrumResourcesPerLink->Allocation(SpectrumRange, DupRes, path, topo->getNumNodes(), par("_FrequencySlotPerLink"));

						//Schedule the message for terminating connection after its life time.
						DupRes->setKind(1);
						ServedSoFarBP = ServedSoFarBP+Breaked.at(i);
						ServedSoFarTD = ServedSoFarTD+DupRes->getRealTrafficDemand();
						SpectrumResourcesPerLink->SaveOnTheExistingConnectionsList(DupRes, path);
						scheduleAt(simTime()+ DupRes->getLifeTime(), DupRes);
					}else
						delete DupRes;
				}

				//Show the result of allocation
				std::vector<long> SpectrumStatus = SpectrumResourcesPerLink->ContinuityConstraint(NumberOfCoresModes*FrequencySlotPerLink, topo->getNumNodes(), path);
				SpectrumResourcesPerLink->ShowSpectrumStatus(SpectrumStatus, NumberOfCoresModes);

				res->setBandwidth(res->getBandwidth()-ServedSoFarBP);
				res->setRealTrafficDemand(res->getRealTrafficDemand()-ServedSoFarTD);
				if(res->getBandwidth() != 0) NotAllocated = true;
				else delete res;

			}else if(Scenario == 2){

				int FJS_JS = par("_FJS_JS");        //Independent Switching = 1, Fractional Joint Switching = 2, Joint Switching = 3.

				int BreakDown;
				std::vector<int> Breaked;
				EV <<"Bandwidth \t" << res->getBandwidth() << endl;

				bool Request_BreakDown_On = par("_Request_BreakDown_On");
				int Max_SpChTRx_Capacity = par("_Max_SpChTRx_Capacity");
				//This part of code is valid if totally independent Sb-Ch allocation is possible.
				/*if(Request_BreakDown_On){
					//Break Down method selection based on the switching paradigm
					if(FJS_JS == 1){
						EV <<"Independent Switching" << endl;
						BreakDown = res->getBandwidth();
						if (BreakDown > 1){
							int Temp = res->getBandwidth();
							while (Temp > 0){
								Breaked.push_back(1);
								Temp = Temp - 1;}
						}else{
							Breaked.push_back(res->getBandwidth());}
					}else if (FJS_JS == 2){
						EV <<"Fractional Joint Switching" << endl;
						double G_FJS = par("_G_FJS");
						BreakDown = res->getBandwidth();
						if (BreakDown > G_FJS){
							int Temp = res->getBandwidth();
							while (Temp > G_FJS){
								Breaked.push_back(G_FJS);
								Temp = Temp - G_FJS;}
							if (Temp > 0)
								Breaked.push_back(Temp);
						}else{
							Breaked.push_back(res->getBandwidth());}
					}else if (FJS_JS == 3){
						EV <<"Joint Switching" << endl;
						BreakDown = res->getBandwidth();
						if (BreakDown > NumberOfCoresModes){
							int Temp = res->getBandwidth();
							while (Temp > NumberOfCoresModes){
								Breaked.push_back(NumberOfCoresModes);
								Temp = Temp - NumberOfCoresModes;}
							if (Temp > 0)
								Breaked.push_back(Temp);
						}else{
							Breaked.push_back(res->getBandwidth());}
					}*/

				//This is valid if we consider the type of TRx that we discussed with Ioannis. Max. # of Sb-Ch per Sp-Ch TRx = NumberOfCoresModes
				if(Request_BreakDown_On){
					BreakDown = res->getBandwidth();
					if (BreakDown > Max_SpChTRx_Capacity){
						int Temp = res->getBandwidth();
						while (Temp > Max_SpChTRx_Capacity){
							Breaked.push_back(Max_SpChTRx_Capacity);
							Temp = Temp - Max_SpChTRx_Capacity;}
						if (Temp > 0)
							Breaked.push_back(Temp);
					}else{
						Breaked.push_back(res->getBandwidth());}

					EV <<"Break Down : \t" << BreakDown << endl;
					EV <<"Sequence: \t";
					for (unsigned int i=0; i<Breaked.size(); i++)
						EV << Breaked.at(i) << " ";
					EV <<"\n\n" << endl;
				}else{
					Breaked.push_back(res->getBandwidth());}

				int ServedSoFarBP = 0;
				double ServedSoFarTD = 0;
				for (unsigned int i=0; i<Breaked.size(); i++)
				{
					std::vector<long> SpectrumStatus = SpectrumResourcesPerLink->ContinuityConstraint(NumberOfCoresModes*FrequencySlotPerLink, topo->getNumNodes(), path);
					SpectrumResourcesPerLink->ShowSpectrumStatus(SpectrumStatus, NumberOfCoresModes);//Show spectrum status.

					Request *DupRes = res->dup();
					DupRes->setBandwidth(Breaked.at(i));
					DupRes->setRealTrafficDemand((double) Breaked.at(i)*((double) res->getRealTrafficDemand()/res->getBandwidth()));

					if(FJS_JS == 2)						//Fractional Joint Switching (G = 3).
					{
						double G_FJS = par("_G_FJS");
						DupRes->setBandwidth(G_FJS*ceil((double)DupRes->getBandwidth()/G_FJS));
						EV <<"Fractional Joint Switching (G = 3) as a result \t : " << DupRes->getBandwidth() <<"\n" <<endl;
					}else if(FJS_JS == 3)				//Joint Switching.
					{
						DupRes->setBandwidth(NumberOfCoresModes*ceil((double)DupRes->getBandwidth()/NumberOfCoresModes));
						EV <<"Joint Switching as a result \t : " << DupRes->getBandwidth() <<"\n" <<endl;}

					//Space Contiguity Constraint.
					std::vector<int> SpectrumRange = SpectrumResourcesPerLink->Space_ContiguityConstraint(SpectrumStatus, DupRes, DupRes->getBandwidth(), NumberOfCoresModes);
					SpectrumResourcesPerLink->Space_ShowSpectrumRange(SpectrumRange, par("_FrequencySlotPerLink"));
					if(SpectrumRange.at(0) != SpectrumRange.at(1))								//if the beginning and ending of the range are not equal means it is possible to allocate connection.
					{
						//Connection allocation. NotAllocated = False if the connection allocate.
						NotAllocated = SpectrumResourcesPerLink->Space_Allocation(SpectrumRange, DupRes, path, topo->getNumNodes(), par("_FrequencySlotPerLink"));

						//Schedule the message for terminating connection after its life time.
						DupRes->setKind(1);
						ServedSoFarBP = ServedSoFarBP+Breaked.at(i);
						ServedSoFarTD = ServedSoFarTD+DupRes->getRealTrafficDemand();
						SpectrumResourcesPerLink->SaveOnTheExistingConnectionsList(DupRes, path);
						scheduleAt(simTime()+ DupRes->getLifeTime(), DupRes);
					}else
						delete DupRes;
				}

				//Show the result of allocation
				std::vector<long> SpectrumStatus = SpectrumResourcesPerLink->ContinuityConstraint(NumberOfCoresModes*FrequencySlotPerLink, topo->getNumNodes(), path);
				SpectrumResourcesPerLink->ShowSpectrumStatus(SpectrumStatus, NumberOfCoresModes);

				res->setBandwidth(res->getBandwidth()-ServedSoFarBP);
				res->setRealTrafficDemand(res->getRealTrafficDemand()-ServedSoFarTD);
				if(res->getBandwidth() != 0) NotAllocated = true;
				else delete res;
			}
		}
		delete topo;

		//Run stop condition. Number of generated connections per run = set value in .ini file.
		if(InformationCollector->NumberOfGeneratedConnections == RequestsPerRun)
		{
			EV <<"////////////////////////////////////////////////////////////////////////////" << endl;
			bool SimulationEnding = false;

			InformationCollector->BlockingProbabilityCalculation();
			if(InformationCollector->BlockingProbabilityPerRun.size() >= (unsigned) NumberOfRuns)
				SimulationEnding = InformationCollector->SimulationEnding();

			if(SimulationEnding) endSimulation();
		}

		scheduleAt(simTime()+ par("_IAT"), msg);

	}else
	{
		//Connection termination.
		Request *Connectiontermination = check_and_cast<Request *>(msg);																				//This is for connections termination.

		cTopology *topo = new cTopology("topo");																										//Topology extraction from .ned fine.
		topo->extractByNedTypeName(cStringTokenizer("Router").asVector());

		double NumberOfCoresModes = par("_NumberOfCoresModes");
		double FrequencySlotPerLink = par("_FrequencySlotPerLink");
		SpectrumResourcesPerLink->ConnectionTermination(Connectiontermination, topo->getNumNodes(), NumberOfCoresModes*FrequencySlotPerLink);			//A search on all elements of array to find terminating connection Id.

		SpectrumResourcesPerLink->RemoveFromTheExistingConnectionsList(Connectiontermination);
		ConnectionReceived(Connectiontermination);							//Received data Counter.
		delete topo;
	}
}

Request * Router::generateMessage()
{
	//Loading data to new connection request: Source, Destination (!= Source), Life time, Requested bandwidth(Gbps)
	//{using a normal or log-normal or any appropriate distributions}.

	cTopology *topo = new cTopology("topo");
	topo->extractByNedTypeName(cStringTokenizer("Router").asVector());

	Request *res = new Request ("Request");
	res->setKind(0);
	res->setSource(getIndex());
	res->setLifeTime(par("_HT"));
	//do{res->setDestination(intuniform(0,topo->getNumNodes()-1));}while(res->getDestination() == getIndex()); // general destination selection;
	do{res->setRealTrafficDemand(ceil(normal(par("_Avr"), 75)));}while(res->getRealTrafficDemand() < 50 || res->getRealTrafficDemand() > 1800);

	int Destin;
	if (getIndex()==3)
		Destin = 0;
	else if (getIndex()==4)
		Destin = 1;
	else if (getIndex()==28)
		Destin = 2;
	else if (getIndex()==27)
		Destin = 3;
	else if (getIndex()==22)
		Destin = 4;
	else if (getIndex()==23)
		Destin = 5;
	else if (getIndex()==7)
		Destin = 6;
	else if (getIndex()==8)
		Destin = 7;
	else if (getIndex()==11)
		Destin = 8;
	else if (getIndex()==10)
		Destin = 9;
	else if (getIndex()==18)
		Destin = 10;
	else if (getIndex()==16)
		Destin = 11;
	else if (getIndex()==14)
		Destin = 12;
	else if (getIndex()==13)
		Destin = 13;

	do{res->setDestination(intuniform(0,13));}while(res->getDestination() == Destin); // Destination selection for TID topology only;

	if(res->getDestination() == 0)
		res->setDestination(3);
	else if(res->getDestination() == 1)
		res->setDestination(4);
	else if(res->getDestination() == 2)
		res->setDestination(28);
	else if(res->getDestination() == 3)
		res->setDestination(27);
	else if(res->getDestination() == 4)
		res->setDestination(22);
	else if(res->getDestination() == 5)
		res->setDestination(23);
	else if(res->getDestination() == 6)
		res->setDestination(7);
	else if(res->getDestination() == 7)
		res->setDestination(8);
	else if(res->getDestination() == 8)
		res->setDestination(11);
	else if(res->getDestination() == 9)
		res->setDestination(10);
	else if(res->getDestination() == 10)
		res->setDestination(18);
	else if(res->getDestination() == 11)
		res->setDestination(16);
	else if(res->getDestination() == 12)
		res->setDestination(14);
	else if(res->getDestination() == 13)
		res->setDestination(13);

	EV <<"Message ID : \t" << res->getId() << endl;
	EV <<"Source : \t \t" << res->getSource() << endl;
	EV <<"Destination : \t" << res->getDestination() << endl;
	EV <<"Life time : \t" << res->getLifeTime() << " (s) " << endl;
	EV <<"Traffic demand : \t" << res->getRealTrafficDemand() << " (Gbps)" << endl;

	delete topo;
	return res;
}

Request * Router::ModulationLevelSeclection(Request *msg, std::vector <int> CurrentPath)
{
	//SuperChannel and SubChannel generation.
	int Scenario = par("_Scenario");			//Over_Spectrum = 1, Over_Space = 2;

	double FSforOneSubBand;
	double TotalSpectrumOfSubBands;
	double MaxSingleSubBandCapacity;
	double TotalSpectrumPerSuperChannel;
	double FS_SubBand = par("_FS_SubBand");
	double SupChannel_GB = par("_SupChannel_GB");
	double GB_ForOneSubBand = par("_SubBand_GB");
	double SubBandSpacSize = par("_SubBandSpacSize");
	double Avg_link_length = par("_Avg_link_length");
	double TheIncomingTraffic = msg->getRealTrafficDemand();
	double FrequencySlotUnitSize = par("_FrequencySlotUnitSize");

	if(Scenario == 1)
		EV << "Scenario  : \t Over Spectrum" << endl;
	else if(Scenario == 2)
		EV << "Scenario  : \t Over Space" << endl;

	//Path length (Km).
	double path_length = (CurrentPath.size()-1)*Avg_link_length;

	if(Scenario == 1){
		EV << "SubBan Spectral occupancy  : \t " << SubBandSpacSize << endl;
		EV << "Frequency Slot Unit Size (subBand level) : \t " << FS_SubBand << endl;
		EV << "Frequency Slot Unit Size (SuperChannel level)  : \t " << FrequencySlotUnitSize << endl;
	} else if(Scenario == 2){
		EV << "WDM ITU-T grids (SuperChannel level)  : \t " << FrequencySlotUnitSize << endl;}

	EV << "\nAvg. Link Length : \t " << Avg_link_length << endl;
	EV << "Path Length  : \t " << path_length << endl;

	//it is Sb-Ch GB calculation as OFC paper.
	//2* is because function returns one side GB value and to have for 2-sides we need to double it.
	//GB_ForOneSubBand = 2*SubBandGB(par("_SubBandResolution"), par("_SubBandXtalk"));

	//Max optical reach calculations.
	double MaxDist_16QAM;
	double MaxDist_8QAM;
	double MaxDist_QPSK;
	double MaxDist_BPSK;

	//Look up table that relates total GB with optical reach.
	if (GB_ForOneSubBand == 18){
		MaxDist_16QAM = 750;
		MaxDist_8QAM =  1700;
		MaxDist_QPSK =  4000;
		MaxDist_BPSK =  7500;
	}else if (GB_ForOneSubBand == 8.625){
		MaxDist_16QAM = 700;
		MaxDist_8QAM =  1400;
		MaxDist_QPSK =  3700;
		MaxDist_BPSK =  6900;
	}else if (GB_ForOneSubBand == 5.5){
		MaxDist_16QAM = 600;
		MaxDist_8QAM =  1200;
		MaxDist_QPSK =  3500;
		MaxDist_BPSK =  6300;
	}else if (GB_ForOneSubBand == 2.375){
		MaxDist_16QAM = 200;
		MaxDist_8QAM =  800;
		MaxDist_QPSK =  3000;
		MaxDist_BPSK =  5750;
	}else if (GB_ForOneSubBand == 0.8){
		MaxDist_16QAM = 0;
		MaxDist_8QAM =  400;
		MaxDist_QPSK =  2400;
		MaxDist_BPSK =  5000;
	}else if (GB_ForOneSubBand == 0){
		MaxDist_16QAM = 0;
		MaxDist_8QAM =  0;
		MaxDist_QPSK =  2050;
		MaxDist_BPSK =  4400;}

	if(Scenario == 2)
	{	MaxDist_16QAM = 750;
		MaxDist_8QAM =  1700;
		MaxDist_QPSK =  4000;
		MaxDist_BPSK =  7500;}


	double Baud_Rate = par("_Baud_rate");							//INSPACE: Sb-Ch Baud Rate = 32 GBaud.
	int Modulation_Format_Selection = par("_Modulation_Format_Selection");

	if(Modulation_Format_Selection == 1){
		//Single modulation format: DP-BPSK.
		EV <<"Modulation Format : \t DP-BPSK" <<endl;
		if (path_length < MaxDist_BPSK)
			MaxSingleSubBandCapacity = 2*Baud_Rate;					//DP-BPSK  (64Gbps)
		else{
		      msg->setBandwidth(0);
		      EV <<"Optical reach is not enough!\n"<<endl;
		      return msg;}
	}else if (Modulation_Format_Selection == 2){
		//Single modulation format: DP-QPSK.
		EV <<"Modulation Format : \t DP-QPSK" <<endl;
		if (path_length < MaxDist_QPSK)
			MaxSingleSubBandCapacity = 4*Baud_Rate;					//DP-QPSK  (128Gbps)
		else{
			  msg->setBandwidth(0);
			  EV <<"Optical reach is not enough!\n"<<endl;
			  return msg;}
	}else if (Modulation_Format_Selection == 3){
		//Single modulation format: DP-8QAM.
		EV <<"Modulation Format : \t DP-8QAM" <<endl;
		if (path_length < MaxDist_8QAM)
			MaxSingleSubBandCapacity = 6*Baud_Rate;					//DP-8QAM  (192Gbps)
		else{
			  msg->setBandwidth(0);
			  EV <<"Optical reach is not enough!\n"<<endl;
			  return msg;}
	}else if (Modulation_Format_Selection == 4){
		//Single modulation format: DP-16QAM.
		EV <<"Modulation Format : \t DP-16QAM" <<endl;
		if (path_length < MaxDist_16QAM)
			MaxSingleSubBandCapacity = 8*Baud_Rate;			  	    //DP-16QAM (256Gbps)
		else{
			  msg->setBandwidth(0);
			  EV <<"Optical reach is not enough!\n"<<endl;
			  return msg;}
	}else if (Modulation_Format_Selection == 5){
		//Modulation selection due to the path length.
		EV <<"Modulation Format : \t DP- BPSK, QPSK, 8QAM, 16QAM" <<endl;
		if (path_length < MaxDist_16QAM)
			MaxSingleSubBandCapacity = 8*Baud_Rate;					//DP-16QAM (256Gbps)
		else if (path_length < MaxDist_8QAM)
			MaxSingleSubBandCapacity = 6*Baud_Rate;					//DP-8QAM  (192Gbps)
		else if (path_length < MaxDist_QPSK)
			MaxSingleSubBandCapacity = 4*Baud_Rate;					//DP-QPSK  (128Gbps)
		else if (path_length < MaxDist_BPSK)
			MaxSingleSubBandCapacity = 2*Baud_Rate;					//DP-BPSK  (64Gbps)
		else{
			msg->setBandwidth(0);
			EV <<"Optical reach is not enough!\n"<<endl;
			return msg;}
	}

	if(Scenario == 1){
		//GB_ForOneSubBand is total GB (2 sides) per sub-Channel.
		//SupChannel_GB is total GB (2 sides) per supper-Channel.
		double NumberOfSubBandsPerSuperChannel = ceil(TheIncomingTraffic/MaxSingleSubBandCapacity);

		if (GB_ForOneSubBand == 0 || GB_ForOneSubBand == 0.8){
			//Grid-less in sub-Channel level.
			FSforOneSubBand = SubBandSpacSize + GB_ForOneSubBand;
			TotalSpectrumOfSubBands = FSforOneSubBand * NumberOfSubBandsPerSuperChannel;
		}else{
			//Grids in sub-Channel level.
			FSforOneSubBand = ceil((SubBandSpacSize + GB_ForOneSubBand) / FS_SubBand);
			TotalSpectrumOfSubBands = FSforOneSubBand * FS_SubBand * NumberOfSubBandsPerSuperChannel;}

		if (SupChannel_GB > GB_ForOneSubBand)
			TotalSpectrumPerSuperChannel = (SupChannel_GB - GB_ForOneSubBand) + TotalSpectrumOfSubBands;				//(12.5-2*GB_ForOneSubBand) guarantees 12.5GHz GB at Sp-Ch level.
		else
			TotalSpectrumPerSuperChannel = TotalSpectrumOfSubBands;

		int RequiredNumberFS = ceil(TotalSpectrumPerSuperChannel/FrequencySlotUnitSize);						//FS number per Sp-Ch.
		msg->setBandwidth(RequiredNumberFS);

		EV <<"Total GB per SubBand : \t" << GB_ForOneSubBand << endl;
		EV << "Max. Capacity of Single SubBand : \t " << MaxSingleSubBandCapacity << endl;
		EV <<"Total spectrum required for SubBands :   " << TotalSpectrumOfSubBands << " (GHz)    which is equal to    " << ceil(TotalSpectrumOfSubBands/FS_SubBand) << "    frequency slots with size of  " << FS_SubBand << " (GHz)" << endl;
		EV <<"After adding GB for Sp-Ch the required spectrum is :  " << TotalSpectrumPerSuperChannel << " (GHz)  which is   " << msg->getBandwidth() <<"  frequency slots with size of " << FrequencySlotUnitSize << " (GHz)\n" <<endl;
	}else if(Scenario == 2){
		//This is for the case of WDM-over-space, where the sub-channel size of spatial super-channels is equal.
		double RequiredNumberFS = ceil(TheIncomingTraffic/MaxSingleSubBandCapacity);
		msg->setBandwidth(RequiredNumberFS);

		EV << "Max. Capacity of Single SubBand : \t " << MaxSingleSubBandCapacity << endl;
		EV <<"Required number of Space Sb-chs : \t " << msg->getBandwidth() <<"\n" <<endl;}

	return msg;
}

std::vector <int> Router::PhatFinder(Request *msg, cTopology *topo)
{
	//Routing problem solution.
	std::vector <int> path;														//local array keeps the Kth shortest path.
	path.push_back(msg->getSource());												//Each path begins with the source node.

	cTopology::Node *SourceNode = topo->getNode(msg->getSource());
	cTopology::Node *DestinationNode = topo->getNode(msg->getDestination());

	do
	{
		topo->calculateUnweightedSingleShortestPathsTo(DestinationNode);
		cTopology::LinkOut *Outlink = SourceNode->getPath(0);
		SourceNode = Outlink->getRemoteNode();
		path.push_back(SourceNode->getModuleId()-2);

	}while(SourceNode->getModuleId() != DestinationNode->getModuleId());		//Each path ends with the destination node.

	return path;
}

void Router::DisplayPath(std::vector<int> path, Request *msg)
{
	EV <<"Path : \t \t \t";
	for(std::vector<int>::iterator it = path.begin(); it != path.end(); ++it)
		EV << *it << " ";
	EV << "\nPath length is : " << (path.size()-1) << "  and message is displayed as : " << (path.size()-1) * msg->getId() << "\n" << endl;

	//InformationCollector->PathProfileIncrement((path.size()-1));
	//InformationCollector->PathProfiledisplay();
}

bool Router::UpdateTopology(std::vector<int> path, cTopology *topo, Request *msg)
{
	//Updating topology for next shortest path calculation. Disable the links in the previously calculated shortest path.
	bool ThereIsNoRoute = false;

	for(unsigned int i=0; i<path.size()-1; i++)										//Disables the links of previous path. Kth Disjoint shortest path.
	{
		cTopology::Node *SourceNode = topo->getNode(path.at(i));
		cTopology::Node *DestinationNode = topo->getNode(path.at(i+1));
		topo->calculateUnweightedSingleShortestPathsTo(DestinationNode);
		cTopology::LinkOut *Outlink = SourceNode->getPath(0);
		Outlink->disable();

	/*	SourceNode = topo->getNode(path.at(i+1));
		DestinationNode = topo->getNode(path.at(i));
		topo->calculateUnweightedSingleShortestPathsTo(DestinationNode);
		Outlink = SourceNode->getPath(0);
		Outlink->disable();*/
	}

	//Checking is there still a path between end nodes or not.

	try
	 {
		cTopology::Node *DestinationNode_test = topo->getNode(msg->getDestination());
		topo->calculateUnweightedSingleShortestPathsTo(DestinationNode_test);
	 }
	catch (std::exception &e)
	 {
		ThereIsNoRoute = true;
	 }

	return ThereIsNoRoute;
}

void Router::ConnectionDrop(Request *msg)
{
	EV<<"\nThe amount of data dropped in this event is    " << msg->getRealTrafficDemand() << "  Gbps\n" << endl;
	InformationCollector->NumberOfDropsIncrement(msg->getRealTrafficDemand());
	InformationCollector->ShowNumberofDrops();
	delete msg;
}

void Router::ConnectionReceived(Request *msg)
{
	double FrequencySlotUnitSize = par("_FrequencySlotUnitSize");
	EV<<"The amount of data received in this event is    " << msg->getRealTrafficDemand() << "  Gbps" << endl;
	EV<<"The spectral occupancy of received data in this event is    " << msg->getBandwidth()*FrequencySlotUnitSize  << "  GHz" << endl;
	InformationCollector->NumberOfReceivesIncrement(msg->getRealTrafficDemand());
	InformationCollector->ReceivesSpectrumIncrement(msg->getBandwidth()*FrequencySlotUnitSize);
	InformationCollector->ShowNumberofReceives();
	InformationCollector->ShowSpectrumofReceives();
	delete msg;
}

double Router::SubBandGB(double SubBandResolution, double SubBandXtalk)
{
	//The GB values here are GB for one side of sub-channel.The addressability is 0.2GHz.
	double SubBandGaurdBand;

	if (SubBandResolution == 0)
		SubBandGaurdBand = 0;
	else if (SubBandResolution == 0.8){
		if(SubBandXtalk == 10)
			SubBandGaurdBand = 0.4;
		else if(SubBandXtalk == 20)
			SubBandGaurdBand = 0.7;
		else if(SubBandXtalk == 30)
			SubBandGaurdBand = 0.81;
		else if(SubBandXtalk == 35)
			SubBandGaurdBand = 0.9;
		else if(SubBandXtalk == 40)
			SubBandGaurdBand = 0.98;
	}else if (SubBandResolution == 1){
		if(SubBandXtalk == 10)
			SubBandGaurdBand = 0.42;
		else if(SubBandXtalk == 20)
			SubBandGaurdBand = 0.75;
		else if(SubBandXtalk == 30)
			SubBandGaurdBand = 1.01;
		else if(SubBandXtalk == 35)
			SubBandGaurdBand = 1.12;
		else if(SubBandXtalk == 40)
			SubBandGaurdBand = 1.22;
	}else if (SubBandResolution == 1.2){
		if(SubBandXtalk == 10)
			SubBandGaurdBand = 0.5;
		else if(SubBandXtalk == 20)
			SubBandGaurdBand = 0.9;
		else if(SubBandXtalk == 30)
			SubBandGaurdBand = 1.21;
		else if(SubBandXtalk == 35)
			SubBandGaurdBand = 1.34;
		else if(SubBandXtalk == 40)
			SubBandGaurdBand = 1.46;
	}else if (SubBandResolution == 1.5){
		if(SubBandXtalk == 10)
			SubBandGaurdBand = 0.61;
		else if(SubBandXtalk == 20)
			SubBandGaurdBand = 1.11;
		else if(SubBandXtalk == 30)
			SubBandGaurdBand = 1.49;
		else if(SubBandXtalk == 35)
			SubBandGaurdBand = 1.66;
		else if(SubBandXtalk == 40)
			SubBandGaurdBand = 1.82;
	}else if (SubBandResolution == 2){
		if(SubBandXtalk == 10)
			SubBandGaurdBand = 0.79;
		else if(SubBandXtalk == 20)
			SubBandGaurdBand = 1.45;
		else if(SubBandXtalk == 30)
			SubBandGaurdBand = 1.97;
		else if(SubBandXtalk == 35)
			SubBandGaurdBand = 2.19;
		else if(SubBandXtalk == 40)
			SubBandGaurdBand = 2.39;
	}else if (SubBandResolution == 2.5){
		if(SubBandXtalk == 10)
			SubBandGaurdBand = 0.95;
		else if(SubBandXtalk == 20)
			SubBandGaurdBand = 1.78;
		else if(SubBandXtalk == 30)
			SubBandGaurdBand = 2.42;
		else if(SubBandXtalk == 35)
			SubBandGaurdBand = 2.7;
		else if(SubBandXtalk == 40)
			SubBandGaurdBand = 2.96;
	}else if (SubBandResolution == 2.75){
		if(SubBandXtalk == 10)
			SubBandGaurdBand = 1.02;
		else if(SubBandXtalk == 20)
			SubBandGaurdBand = 1.94;
		else if(SubBandXtalk == 30)
			SubBandGaurdBand = 2.65;
		else if(SubBandXtalk == 35)
			SubBandGaurdBand = 2.95;
		else if(SubBandXtalk == 40)
			SubBandGaurdBand = 3.23;
	}else if (SubBandResolution == 3.125){
		if(SubBandXtalk == 10)
			SubBandGaurdBand = 1.13;
		else if(SubBandXtalk == 20)
			SubBandGaurdBand = 2.17;
		else if(SubBandXtalk == 30)
			SubBandGaurdBand = 2.97;
		else if(SubBandXtalk == 35)
			SubBandGaurdBand = 3.32;
		else if(SubBandXtalk == 40)
			SubBandGaurdBand = 3.64;
	}

	return SubBandGaurdBand;
}

void Router::finish()
{
	//the results will be at a .txt file.
	if(getIndex() == 0)
	{
		int Scenario = par("_Scenario");
		double Load = par("_Load");
		double GB_ForOneSubBand = par("_SubBand_GB");
		double Avglinklength = par("_Avg_link_length");
		double holding = par("_HoldingTime");
		double FrequencySlotUnitSize = par("_FrequencySlotUnitSize");
		double FSsubBand = par("_FS_SubBand");
		//double SubBandResolution = par("_SubBandResolution");
		//double SubBandXtalk = par("_SubBandXtalk");
		double NumberOfCoresModes = par("_NumberOfCoresModes");
		int FrequencySlotPerLink = par("_FrequencySlotPerLink");
		bool Request_BreakDown_On = par("_Request_BreakDown_On");
		int Modulation_Format_Selection = par("_Modulation_Format_Selection");
		std::vector<double> DataReadyForDisplay = InformationCollector->Finaldisplay();

		cTopology *topo = new cTopology("topo");
		topo->extractByNedTypeName(cStringTokenizer("Router").asVector());

		recordScalar("Network blocking probability:    " , DataReadyForDisplay.at(0));
		recordScalar("Spectrum utilization:    " , DataReadyForDisplay.at(5));

		recordScalar("GB For One Sub-Band :    ", GB_ForOneSubBand);
		recordScalar("Frequency slot size superChannnel level :    ", FrequencySlotUnitSize);
		recordScalar("Frequency slot size SubBand level :    ", FSsubBand);
		//recordScalar("Sub Band Resolution :    ", SubBandResolution);
		//recordScalar("Sub Band Xtalk :    ", SubBandXtalk);
		recordScalar("Number of nodes in the network:    ", topo->getNumNodes());
		recordScalar("Average Link Length:    ", Avglinklength);
		recordScalar("Total available spectrum per link:    ", (FrequencySlotPerLink*FrequencySlotUnitSize)/1000);
		recordScalar("Number of Cores/Modes:    ", NumberOfCoresModes);
		recordScalar("Average offered load per node (live connections per node):    ", Load);
		recordScalar("Average bit rate per connection (Gbps):    ", InformationCollector->AverageBitRatePerConnection);
		recordScalar("The connection holding time (s):    ", holding);
		recordScalar("Connection intervals (s):    ", holding/Load);
		recordScalar("Average number of generated connection per run:    ", DataReadyForDisplay.at(1));
		recordScalar("The average amount of data drop per run (Gbps):    ", DataReadyForDisplay.at(2));
		recordScalar("The average amount of received data per run (Gbps):    ", DataReadyForDisplay.at(3));
		recordScalar("The average spectrum occupancy per run (GHz):    ", DataReadyForDisplay.at(4));

		if(Request_BreakDown_On)
			recordScalar("Request BreakDown is On", 1);
		else
			recordScalar("Request BreakDown is Off", 0);

		if(Modulation_Format_Selection == 1)
			recordScalar("Modulation Format is DP-BPSK", 1);
		else if (Modulation_Format_Selection == 2)
			recordScalar("Modulation Format is DP-QPSK", 2);
		else if (Modulation_Format_Selection == 3)
			recordScalar("Modulation Format is DP-8QAM", 3);
		else if (Modulation_Format_Selection == 4)
			recordScalar("Modulation Format is DP-16QAM", 4);
		else if (Modulation_Format_Selection == 5)
			recordScalar("Modulation Format is DP- BPSK, QPSK, 8QAM, 16QAM", 5);

		if(Scenario == 1)
			recordScalar("Scenario Over Spectrum", 1);
		else if (Scenario == 2)
			recordScalar("Scenario Over Space", 2);

		delete topo;
	}
}
