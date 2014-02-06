/*
Program:Main.cpp
Author:Jacob Americano
Date:Nov, 27 2013
Description:read 2 files that contain coordinates of residences and foodbanks in Toronto. Determine the shortest straight line distance between a residence and a foodbank then place them into categories based on distances. Then output this categorization to the console.
*/

#include <iostream>
#include <iomanip>
#include <fstream>
#include <vector>
#include <string>
#include <mpi.h>
#include <cmath>

using namespace std;

const int BYTES = 24; 

const char* resFile = "residences.dat";

const char* foodFile = "foodbanks.dat";

/*Structure for storing coordinates*/
typedef struct{ double x, y; } Coords;

/*Structure for categorization*/
struct container
{
	int lessThanOne, lessThanTwo, lessThanFive, greaterThanFive, records;

	double percLessThanOne, percLessThanTwo, percLessThanFive, percGreaterThanFive;

	container() : lessThanOne( 0 ), lessThanTwo( 0 ), lessThanFive( 0 ), greaterThanFive( 0 ), records( 0 ), percLessThanOne( 0 ), percLessThanTwo( 0 ), percLessThanFive( 0 ), percGreaterThanFive( 0 )  { }

};

double tTime;

int numAddrss = 0;

container contain;

MPI_Datatype container_type;

vector<Coords> banks;

/*
* Method: fileSize()
* Author:Jacob Americano
* Date:Nov, 27 2013
* Purpose: determine the files size
*/
streampos fileSize( const char* path )
{        
	streampos fsize = 0;

	ifstream file( path, ios::binary );

	fsize = file.tellg();

	file.seekg( 0, ios::end );

	fsize = file.tellg() - fsize;

	file.close();

	return fsize;
}

/*
* Name: processResidences()
* Author:Jacob Americano
* Date:Nov, 27 2013
* Purpose: Process residence coordinates and determine the shortest distance to a foodbank.  Store the info and report to the master process. 
*			The master process will display the findings.
*/
void process( int rank, int numProcs ) {

	try {

		ifstream infile( resFile );

		streampos size = fileSize( resFile );

		int numRes = (int)size / BYTES + 1;

		int pos = rank;

		while( pos <  numRes ) {

			infile.seekg( pos * BYTES );

			numAddrss++;

			Coords x_y;

			infile >> x_y.x >> x_y.y;

			container temp;

			temp.lessThanOne = 0;

			for( unsigned int i = 0; i < banks.size(); ++i ) {

				double km = sqrt(  pow(( x_y.x - banks[i].x ), 2) + pow( ( x_y.y - banks[i].y ),2 ) ) / 1000;

				if( km <= 1.0 ) {

					temp.lessThanOne++;

					break;

				} else if( km > 1.0 && km <= 2.0 ) 

					temp.lessThanTwo++;

				else if( km > 2.0 && km <= 5.0 ) 

					temp.lessThanFive++;

				else 

					temp.greaterThanFive++;

			}

			if( temp.lessThanOne > 0 )

				contain.lessThanOne++;

			else if( temp.lessThanTwo > 0 )

				contain.lessThanTwo++;

			else if( temp.lessThanFive > 0 )

				contain.lessThanFive++;

			else

				contain.greaterThanFive++;

			contain.records++;

			pos += numProcs;
		}

		contain.percLessThanOne = (static_cast<double>( contain.lessThanOne ) / numAddrss) * 100;

		contain.percLessThanTwo = (static_cast<double>( contain.lessThanTwo )/ numAddrss) * 100;

		contain.percLessThanFive = (static_cast<double>( contain.lessThanFive ) / numAddrss) * 100;

		contain.percGreaterThanFive = (static_cast<double>( contain.greaterThanFive ) / numAddrss) * 100;

		container* recvCounter;

		recvCounter = ( container* ) malloc( numProcs * sizeof( container ) );

		if (rank == 0) {

			tTime = MPI_Wtime() - tTime;

			MPI_Gather( &contain, 1, container_type, recvCounter, 1, container_type, 0, MPI_COMM_WORLD);

			int zeroToOneTotal = 0, oneToTwoTotal = 0, twoToFiveTotal = 0, greaterThanFiveTotal = 0, recordsTotal = 0;

			double perZeroToOneTotal, perOneToTwoTotal, perTwoToFiveTotal, perGreaterThanFiveTotal;

			cout << "Proximity of Residential Addresses to Foodbanks" << endl;

			cout << "-----------------------------------------------\n" << endl;

			cout << setw(27) << left << "Number of processes:" << numProcs << endl;

			cout << setw(27) << left << "Elapsed time in seconds:" << tTime << "\n" << endl;

			for( int p = 0; p < numProcs; ++p ) {

				cout << "Process #" << p + 1 << " Results for " << recvCounter[p].records << " Addresses\n" << endl;

				cout << setw(20) << left << "Nearest Foodbank (km)" << setw(20) << right << "# of Addresses" << setw(20) << "% of Addresses" << endl;

				cout << setw(20) << left << "---------------------" << setw(20) << right << "--------------" << setw(20) << "--------------" << endl;

				cout << setw(12) << right << "0-1" << setw(23) << right << recvCounter[p].lessThanOne << setw(20) <<  recvCounter[p].percLessThanOne << endl;

				cout << setw(12) << right << "1-2" << setw(23) << right << recvCounter[p].lessThanTwo << setw(20) << recvCounter[p].percLessThanTwo << endl;

				cout << setw(12) << right << "2-5" << setw(23) << right << recvCounter[p].lessThanFive << setw(20) << recvCounter[p].percLessThanFive << endl;

				cout << setw(12) << right << "> 5" << setw(23) << right << recvCounter[p].greaterThanFive << setw(20) << recvCounter[p].percGreaterThanFive << "\n" << endl;

				zeroToOneTotal += recvCounter[p].lessThanOne;

				oneToTwoTotal += recvCounter[p].lessThanTwo;

				twoToFiveTotal += recvCounter[p].lessThanFive;

				greaterThanFiveTotal += recvCounter[p].greaterThanFive;

				recordsTotal += recvCounter[p].records;

			}

			perZeroToOneTotal = (static_cast<double>( zeroToOneTotal ) / recordsTotal) * 100;

			perOneToTwoTotal = (static_cast<double>( oneToTwoTotal )/ recordsTotal) * 100;

			perTwoToFiveTotal = (static_cast<double>( twoToFiveTotal ) / recordsTotal) * 100;

			perGreaterThanFiveTotal = (static_cast<double>( greaterThanFiveTotal ) / recordsTotal) * 100;

			cout << "Aggregate Results for all " << recordsTotal << " Addresses\n" << endl;

			cout << setw(20) << left << "Nearest Foodbank (km)" << setw(20) << right << "# of Addresses" << setw(20) << "% of Addresses" << endl;

			cout << setw(20) << left << "---------------------" << setw(20) << right << "--------------" << setw(20) << "--------------" << endl;

			cout << setw(12) << right << "0-1" << setw(23) << right << zeroToOneTotal << setw(20) <<  perZeroToOneTotal << endl;

			cout << setw(12) << right << "1-2" << setw(23) << right << oneToTwoTotal << setw(20) << perOneToTwoTotal << endl;

			cout << setw(12) << right << "2-5" << setw(23) << right << twoToFiveTotal << setw(20) << perTwoToFiveTotal << endl;

			cout << setw(12) << right << "> 5" << setw(23) << right << greaterThanFiveTotal << setw(20) << perGreaterThanFiveTotal << "\n" << endl;

		} else 

			MPI_Gather( &contain, 1, container_type, recvCounter, 1, container_type, 0, MPI_COMM_WORLD);

	} catch(exception ex) { cerr << ex.what() << endl; }

}

/*
* Method: main
* Author:Jacob Americano
* Date:Nov, 27 2013
*/

int main( int argc, char* argv[] ) {

	if( MPI_Init( &argc, &argv ) == MPI_SUCCESS ) {

		int numProcs, rank;

		MPI_Comm_size( MPI_COMM_WORLD, &numProcs );

		MPI_Comm_rank( MPI_COMM_WORLD, &rank );

		int blocklens[] = {5, 4};

		MPI_Aint indices[2];

		indices[0] = 0;

		MPI_Type_extent(MPI_DOUBLE, &indices[1]);

		indices[1] *= 3;

		MPI_Datatype oldtypes[] = { MPI_INT, MPI_DOUBLE};

		MPI_Type_struct(2, blocklens, indices, oldtypes, &container_type);

		MPI_Type_commit(&container_type);

		if (rank == 0 ){ tTime = MPI_Wtime(); }

		ifstream infile( foodFile );

		if( !infile ) { 

			cout << "Could not open file: " << foodFile << endl;

			return 1;

		} 

		while( !infile.eof() ) {

			Coords bank;

			infile >> bank.x >> bank.y;

			banks.push_back( bank );

		}

		process( rank, numProcs );

		MPI_Type_free(&container_type);

		MPI_Finalize();

	}

	return EXIT_SUCCESS;
}