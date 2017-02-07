message Request
{ 
    int Source; 				//Source node identifier.
    int Bandwidth;				//Connection’s bandwidth request (Number of frequency slots).
    int Destination; 			//Destination node identifier.
    double LifeTime;    		//Connection’s life time (s). 
	double RealTrafficDemand;	//Real traffic demand. 
}
