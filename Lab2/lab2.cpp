//Mark Mukiiza 
//C00166672
#pragma once
#include <iostream>
#include <stdio.h>
using namespace std;
// dont use cout in any of the functions apart from PrintArray();
//Find the largest value in the entire array and replace all occurrences with - 1. Return the largest value found.
int ReplaceLargestValue(int array[4][6])
{
	int largest = array[0][0];
	for (int row = 0; row < 4; row++) {
		for (int col = 0; col < 6; col++) {
			if (array[row][col] > largest) {
				largest = array[row][col];
			}
		}
	}
	for (int row = 0; row < 4; row++) {
		for (int col = 0; col < 6; col++) {
			if (array[row][col] == largest) {
				array[row][col] = -1;
			}
		}
	}

	return largest;
}

//Count all zero values and replace them with - 1. Return the count of zeros.
int CountAndReplaceZeros(int array[4][6])
{
	int count = 0;
	for (int row = 0; row < 4; row++) {
		for (int col = 0; col < 6; col++) {
			if (array[row][col] == 0) {
				count++;
				array[row][col] = -1;
			}
		}
	}
	return count;
}

//Find any horizontal sequence of 3 consecutive cells matching the given pattern.Replace with - 1 and return matches found.

int ReplaceRowPattern(int array[4][6], int pattern[3])
{
	int matches = 0;

	for (int row = 0; row < 4; row++) {
		int col = 0;
		while (col <= 6 - 3) { 
			bool ok = true;
			for (int innerIndex = 0; innerIndex < 3; innerIndex++) {
				if (array[row][col + innerIndex] != pattern[innerIndex]) {
					ok = false;
					break;
				}
			}

			if (ok) {
				for (int innerIndex = 0; innerIndex < 3; innerIndex++) {
					array[row][col + innerIndex] = -1;
				}
				matches++;
				col += 3;
			}
			else {
				col++;
			}
		}
	}

	return matches;
}
//Swap two specified rows in the array.
void SwapRows(int array[4][6], int row1, int row2)
{

}

//Calculate and return the sum of all values in specified column(ignore - 1 values).
int FindColumnSum(int array[4][6], int col)
{
	return 0;
}


//Replace all border elements(first / last row, first / last column) with - 1. Return count replaced.
int ReplaceBorder(int array[4][6])
{
	return 0;
}

//Count how many different unique values exist in the array(ignore 0 and -1).
int CountUniqueValues(int array[4][6])
{
	return 0;
}
//Reverse each row of the array(first element becomes last, etc.).
void ReverseArrayRows(int array[4][6])
{

}

//Find and return the value that appears most frequently(ignore 0 and -1).
int FindMostFrequent(int array[4][6])
{
	return 0;
}

//Shift all elements one position to the right.Last column moves to first column.
void ShiftArrayRight(int array[4][6])
{

}

int ReplaceWithMinusOne(int array[4][6])
{
	return 0;
}
void FallDownAndReplace(int array[4][6])
{

}
void PrintArray(int array[4][6])
{
	for (int row = 0; row < 4; row++) {
		for (int col = 0; col < 6; col++) {
			cout << array[row][col] << " ";
		}
		cout << endl;
	}
	cout << endl;
}
void FindAllVariables(char variables[1000], char code[1000])
{

}
void FindAllVariablesInScope(char variables[1000], char code[1000], int lineNumber)
{

}
// add your tests here.
int main()
{
	cout << "Find the largest value in the entire array and replace all occurrences with - 1. Return the largest value found.\n";
	int array[4][6] = { {0,0,3,1,3,4},
					   {0,0,2,3,4,3},
					   {0,0,1,3,3,2},
					   {0,0,1,1,1,1} };
	cout << "Before\n";
	PrintArray(array);
	int largest = ReplaceLargestValue(array);
	cout << "After\n";
	PrintArray(array);
	cout << "Largest number in the array was: " << largest << endl;
	cout << endl;

	cout << "Count all zero values and replace them with - 1. Return the count of zeros.\n";
	int array1[4][6] = { {0,0,3,1,3,4},
					   {0,0,2,3,4,3},
					   {0,0,1,3,3,2},
					   {0,0,1,1,1,1} };
	cout << "Before\n";
	PrintArray(array1);
	int zeroCount = CountAndReplaceZeros(array1);
	cout << "After\n";
	PrintArray(array1);
	cout << "Number of zeros in the array was: " << zeroCount << endl;
	cout << endl;
	//int score = ReplaceWithMinusOne(array);
	
	/*
	FallDownAndReplace(array);
	char code[1000] = "";

	char variables[1000] = "";

	FindAllVariables(variables, code);
	FindAllVariablesInScope(variables, code, 12);
	*/

}
