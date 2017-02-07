simple Router
{
    parameters:
        @display("i=abstract/router");
          
        //Scenario.
        int _Scenario;															//Over_Spectrum = 1, Over_Space = 2.
        int _FJS_JS; 															//Independent Switching = 1, Fractional Joint Switching = 2, Joint Switching = 3.
        double _G_FJS;															//Group of Fractional Joint Switching.
        bool _Request_BreakDown_On;												//Request Break Down, on/off.
        int _Modulation_Format_Selection;										//Modulation_Format_Selection. DP-BPSK = 1, DP-QPSK = 2, DP-8QAM = 3, DP-16QAM = 4, ALL = 5.			
        int _Max_SpChTRx_Capacity;												//Maximum capacity of one Sp-Ch TRx in terms of number of Sb-Chs. 
        
        //Modulation selection required info. 
        //double _SubBandResolution;
        //double _SubBandXtalk;
              	
        double _SubBand_GB;														//Total required GB per SubBand (GHz).
        double _SupChannel_GB;													//Total required GB per Super-Channel (GHz).
        double _SubBandSpacSize;												//SubBand spectral occupancy (GHz). 
        double _Avg_link_length;												//Avg. link length (Km).
        double _Baud_rate;														//Sb-Ch Baud rate.
        
        //Information about number of runs and number of generated connections per run.  
    	int _NumberOfRuns;														//Number of runs.
    	int _RequestsPerRun;													//Generated connections per run.
    	
    	//Information about total spectrum per link and frequency slot size (Hz).     	
    	double _TotalSpectrum;					            					//Total available spectrum per link (THz).
    	double _FrequencySlotUnitSize;											//Frequency Slot Unit Size (GHz) supperchannel level.
    	double _FrequencySlotPerLink = _TotalSpectrum/_FrequencySlotUnitSize;	//Frequency Slot Per Link.
    	double _FS_SubBand;														//Frequency Slot subBand level. 
    	double _NumberOfCoresModes;												//Number of Cores and Modes.
    	
    	//Information about the offered load, average number of connections per node plus average bit rate per connection.  
    	double _Avr;															//This number is used in a uniform/lognormal distribution in order to generate different bit rate per connection.  
    	double _Load;															//Load per node in Erlangs.
    	double _HoldingTime @unit(s);											//Mean connection holding time (s).
    	volatile double _HT @unit(s) = exponential(_HoldingTime);				//Upon arrival of the new connection request, a randomly generated holding time is calculated. 
    	volatile double _IAT @unit(s) = exponential(_HoldingTime/_Load);		//Connections interarrival time (IAT) = Holding time / Load. 
 
    gates:
        inout link[];
}

network TID
{
    submodules:
        router[30]: Router;
        
    connections:
        router[0].link++ <--> router[1].link++;
        router[0].link++ <--> router[2].link++;
        router[0].link++ <--> router[3].link++;
        router[1].link++ <--> router[2].link++;
        router[1].link++ <--> router[11].link++;
        router[2].link++ <--> router[5].link++;
        router[3].link++ <--> router[4].link++;
        router[3].link++ <--> router[5].link++;
        router[4].link++ <--> router[7].link++;
        router[4].link++ <--> router[24].link++;
        router[4].link++ <--> router[26].link++;
        router[5].link++ <--> router[8].link++;
        router[5].link++ <--> router[9].link++;
        router[5].link++ <--> router[11].link++;
        router[6].link++ <--> router[7].link++;
        router[6].link++ <--> router[8].link++;
        router[6].link++ <--> router[19].link++;
        router[6].link++ <--> router[24].link++;
        router[7].link++ <--> router[8].link++;
        router[8].link++ <--> router[9].link++;
        router[8].link++ <--> router[17].link++;
        router[9].link++ <--> router[10].link++;
        router[9].link++ <--> router[15].link++;
        router[10].link++ <--> router[11].link++;
        router[10].link++ <--> router[12].link++;
        router[10].link++ <--> router[15].link++;
        router[12].link++ <--> router[13].link++;
        router[12].link++ <--> router[14].link++;
        router[12].link++ <--> router[15].link++;
        router[13].link++ <--> router[14].link++;
        router[13].link++ <--> router[16].link++;
        router[14].link++ <--> router[16].link++;
        router[15].link++ <--> router[16].link++;
        router[16].link++ <--> router[17].link++;
        router[16].link++ <--> router[18].link++;
        router[17].link++ <--> router[18].link++;
        router[17].link++ <--> router[19].link++;
        router[18].link++ <--> router[19].link++;
        router[18].link++ <--> router[20].link++;
        router[19].link++ <--> router[20].link++;
        router[19].link++ <--> router[25].link++;
        router[20].link++ <--> router[21].link++;
        router[21].link++ <--> router[22].link++;
        router[21].link++ <--> router[23].link++;
        router[22].link++ <--> router[23].link++;
        router[22].link++ <--> router[25].link++;
        router[22].link++ <--> router[27].link++;
        router[23].link++ <--> router[29].link++;
        router[24].link++ <--> router[25].link++;
        router[24].link++ <--> router[26].link++;
        router[24].link++ <--> router[27].link++;
        router[26].link++ <--> router[27].link++;
        router[26].link++ <--> router[28].link++;
        router[27].link++ <--> router[28].link++;
        router[27].link++ <--> router[29].link++;
        router[28].link++ <--> router[29].link++;      
}



