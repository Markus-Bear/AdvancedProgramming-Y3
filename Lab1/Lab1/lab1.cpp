
#include <iostream>
#include <stdio.h>
using namespace std;

bool isLeapYear(int year)
{
	if (year % 4 != 0)
	{
		return false;
	}
	else if (year % 100 != 0)
	{
		return true;
	}
	else if (year % 400 != 0)
	{
		return true;
	}
	else
	{
		return false;
	}
		

}
int Reversed(int testNumber)
{
	int reverse = 0;
	int value = testNumber;

	while (testNumber != 0)
	{
		int remainder = testNumber % 10;
		reverse = reverse * 10 + remainder;
		testNumber /= 10;
	}
	return reverse;
}
bool isAPalindrome(int testNumber)
{
	if (testNumber == Reversed(testNumber))
	{
		return true;
	}
	else
	{
		return false;
	}
}
bool isAPrimeNumber(int numbertoTest)
{
	int divCount = 0;
	if (numbertoTest <= 1)
	{
		return false;
	}
	else
	{
		for (int index = 1; index <= numbertoTest; index++)
		{
			if (numbertoTest % index == 0)
			{
				divCount++;
			}
		}

		if (divCount > 2)
		{
			return false;
		}
		else
		{
			return true;
		}
	}
	
}
int convertBinarytoDecimal(int binaryNumber)
{
	int decimal = 0;
	int base = 1;

	while (binaryNumber > 0)
	{
		int lastDigit = binaryNumber % 10;
		binaryNumber = binaryNumber / 10;
		decimal += lastDigit * base;
		base *= 2;
	}
	return decimal;
}
int input5CharsConvertToInt()
{
	int returnInt = 0;
	char inputChar;
	for (int i = 0; i < 5; i++)
	{
		cin >> inputChar;
		
		if(inputChar < '0' || inputChar > '9')
		{
			return 0;
		}

		returnInt = returnInt * 10 + (inputChar - '0');

	}
	return returnInt;
}
void drawRightAngledTriangle()
{
	for (int index = 1; index < 5; index++)
	{
		for (int innerIndex = 1; innerIndex <= index; innerIndex++)
		{
			cout << "A ";
		}

		cout << endl;
	}
}
void drawIsocelesTriangle()
{
	for (int index = 1; index < 8; index++)
	{
		int count;
		if (index <= 4)
		{
			count = index;
		}
		else
		{
			count = 8 - index;
		}
		for (int innerIndex = 1; innerIndex <= count; innerIndex++)
		{
			cout << "A ";
		}

		cout << endl;
	}
}
void drawIsocelesTriangle2()
{
	int counts[7] = { 1, 2, 3, 4, 3, 2, 1 };

	for (int index = 0; index < 7; index++) {
		for (int innerIndex = 0; innerIndex < counts[index]; innerIndex++) {
			std::cout << "A";
		}
		std::cout << '\n';
	}
}

int find(int size, int arr[], int toFind)
{
	
	for (int index = 0; index < size; index++)
	{
		if (toFind == arr[index])
		{
			
			return index;
		}
			
	}
	return -1;
}
int find2ndLargest(int size, int arr[])
{
	int largest = arr[0];
	int secondLargest = 0;
	for (int index = 1; index < size; index++) {
		if (arr[index] > largest) {
			largest = arr[index];
		}
	}

	for (int index = 0; index < size; index++) {
		if (arr[index] > secondLargest && arr[index] < largest) {
			secondLargest = arr[index];
		}
	}
	if (secondLargest > 0) {
		return secondLargest;
	}
	else {
		return -1;
	}
	
}
void copyArraytoArray(int size, int arr1[], int arr2[])
{
	for (int index = 0; index < size; index++) {
		arr2[index] = arr1[index];
	}
}
bool insertElement(int& size, int& count, int arr[], int elementToInsert, int insertIndex)
{
	if (count >= size || insertIndex < 0 || insertIndex > count) {
		return false;
	}

	for (int index = count; index > insertIndex; index--) {
		arr[index] = arr[index - 1];
	}

	arr[insertIndex] = elementToInsert;

	count++;

	return true;
}
bool deleteElement(int& size, int& count, int arr[], int deleteIndex)
{
	if (deleteIndex < 0 || deleteIndex >= count || count == 0) {
		return false;
	}

	for (int index = deleteIndex; index < count - 1; index++) {
		arr[index] = arr[index + 1];
	}

	count--;

	return true;
}
int frequencyCount(int size, int arr[], int value)
{
	int count = 0;
	for (int index = 0; index < size; index++) {
		if (value == arr[index]) {
			count++;
		}
	}
	return count;
}
int countDuplicates(int size, int arr[])
{
	int distinctDuplicateCount = 0;

	for (int index = 0; index < size; index++) {
		bool isFirstOccurrence = true;

		for (int innerIndex = 0; innerIndex < index; innerIndex++) {
			if (arr[index] == arr[innerIndex]) {
				isFirstOccurrence = false;
				break;
			}
		}

		if (isFirstOccurrence) {
			bool hasDuplicate = false;

			for (int innerIndex = index + 1; innerIndex < size; innerIndex++) {
				if (arr[index] == arr[innerIndex]) {
					hasDuplicate = true;
					break;
				}
			}

			if (hasDuplicate) {
				distinctDuplicateCount++;
			}
		}
	}

	return distinctDuplicateCount;
}
void reverse(int size, int arr[])
{
	int temp = 0;
	for (int index = 0; index < size; index++) {
		temp = arr[index];
		arr[index] = arr[size - 1];
		arr[size - 1] = temp;
		size--;
	}
	return;
}
int rotateLeft(int size, int arr[]) {
	if (size <= 1) {
		return 0;
	}

	int first = arr[0]; 

	for (int index = 0; index < size - 1; index++) {
		arr[index] = arr[index + 1];
	}

	arr[size - 1] = first;

	return 1; 
}
bool twoMovies(int flightLength, int movieLengths[], int size)
{
	if (size < 2) {
		return false;
	}

	for (int index = 0; index < size - 1; index++) {
		for (int innerIndex = index + 1; innerIndex < size; innerIndex++) {
			if (movieLengths[index] + movieLengths[innerIndex] == flightLength) {
				return true; 
			}
		}
	}
	return false;
}
int wordCounter(int size, char characters[])
{
	int count = 0;
	bool inWord = false;

	for (int index = 0; index < size; index++) {
		if ((characters[index] >= 'A' && characters[index] <= 'Z') ||
			(characters[index] >= 'a' && characters[index] <= 'z')) {

			if (!inWord) {
				count++;
				inWord = true;
			}
		}
		else {
			inWord = false;
		}
	}
	return count;
}


int main()
{
	cout << "Question 1\n";
	cout << "1. Leap Year. \n";
	if (isLeapYear(2025))
	{
		cout << "Yes.\n";
	}
	else
	{
		cout << "No.\n";
	}
	if (isLeapYear(2016))
	{
		cout << "Yes.\n";
	}
	else
	{
		cout << "No.\n";
	}

	cout << endl;
	cout << "Question 2\n";
	cout << "2. Palindrome.\n";
	if (!isAPalindrome(1213))
	{
		cout << "Yes.\n";
	}
	else
	{
		cout << "No.\n";
	}
	if (isAPalindrome(121))
	{
		cout << "Yes.\n";
	}
	else
	{
		cout << "No.\n";
	}

	cout << endl;
	cout << "Question 3\n";
	cout << "3. Prime.\n";
	if (isAPrimeNumber(3))
	{
		cout << "Yes.\n";
	}
	else
	{
		cout << "No.\n";
	}

	if (isAPrimeNumber(2147))
	{
		cout << "Yes.\n";
	}
	else 
	{
		cout << "No.\n";
	}

	cout << endl;
	cout << "Question 4\n";
	cout << "Binary to Decimal.\n";
	if (convertBinarytoDecimal(110) == 6)
	{
		cout << "Yes.\n";
	}
	else 
	{
		cout << "No.\n";
	}
	if (convertBinarytoDecimal(111) == 7)
	{
		cout << "Yes.\n";
	}
	else {
		cout << "No.\n";
	}

	cout << endl;
	cout << "Question 5\n";
	cout << "5 chars to int.\n";
	int convertedInt = input5CharsConvertToInt();
	cout << convertedInt << endl;
	
	cout << endl;
	cout << "Question 6\n";
	cout << "Triangles.\n";
	drawRightAngledTriangle();
	
	cout << endl;
	cout << "Question 7\n";
	drawIsocelesTriangle();
	
	cout << endl;
	cout << "Question 8\n";
	const int sizeOfArray = 5;
	int numArray[sizeOfArray] = { 1,8,3,6,5 };
	int findNum = 6;
	cout << "The index position of " << findNum << " in the array is " << find(sizeOfArray, numArray, findNum) << endl;
	cout << endl;
	
	cout << "Question 9\n";
	cout << "The second largest number in the array is " << find2ndLargest(sizeOfArray, numArray) << endl;
	int array1[sizeOfArray] = { 1,2,3,4,5 };
	int array2[sizeOfArray] = { 0 };
	
	cout << endl;
	cout << "Question 10\n";
	cout << "Before copy\n";
	cout << "Array 1: ";
	for (int index = 0; index < sizeOfArray; index++) {
		cout << array1[index] << " ";
	}
	cout << endl;

	cout << "Array 2: ";
	for (int index = 0; index < sizeOfArray; index++) {
		cout << array2[index] << " ";
	}
	cout << endl;
	copyArraytoArray(sizeOfArray, array1, array2);
	
	cout << "After copy\n";
	cout << "Array 2: ";
	for (int index = 0; index < sizeOfArray; index++) {
		cout << array2[index] << " ";
	}

	cout << endl;
	cout << "Question 11\n";
	int arrayQEleven[8] = { 1,2,3,4 };
	int qElevenSize = 8;
	int qElevenCount = 4;

	cout << "Array: ";
	for (int index = 0; index < qElevenCount; index++) {
		cout << arrayQEleven[index] << " ";
	}
	cout << endl;
	cout << endl;
	cout << "Front insert.\n";
	if (insertElement(qElevenSize, qElevenCount, arrayQEleven, 6, 0)) {
		cout << "Inserted 6 at index 0: ";
		for (int index = 0; index < qElevenCount; index++) {
			cout << arrayQEleven[index] << " ";
		}
		cout << endl;
	}
	cout << endl;
	cout << "Rear insert.\n";
	if (insertElement(qElevenSize, qElevenCount, arrayQEleven, 7, qElevenCount)) {
		cout << "Inserted 7 at rear of array: ";
		for (int index = 0; index < qElevenCount; index++) {
			cout << arrayQEleven[index] << " ";
		}
		cout << endl;
	}

	cout << endl;
	cout << "Question 12\n";
	int arrayQTwelve[8] = { 1,2,3,4 };
	int qTwelveSize = 8;
	int qTwelveCount = 4;

	cout << "Array\n";
	for (int index = 0; index < qTwelveCount; index++) {
		cout << arrayQTwelve[index] << " ";
	}
	cout << endl;
	cout << endl;
	cout << "Front delete.\n";
	if (deleteElement(qTwelveSize, qTwelveCount, arrayQTwelve, 0)) {
		cout << "Deleted at front of array: ";
			for (int index = 0; index < qTwelveCount; index++) {
				cout << arrayQTwelve[index] << " ";
			}
		cout << endl;
	}
	cout << endl;
	cout << "Rear delete.\n";
	if (deleteElement(qTwelveSize, qTwelveCount, arrayQTwelve, qTwelveCount - 1)) {
		cout << "Deleted at rear of array: ";
		for (int index = 0; index < qTwelveCount; index++) {
			cout << arrayQTwelve[index] << " ";
		}
		cout << endl;
	}
	
	cout << endl;
	cout << "Question 13\n";
	const int qThirteenSize = 8;
	int searchValue = 2;
	int arrayQThirteen[qThirteenSize] = { 1,2,3,2,4,5,2,6 };
	cout << "The value " << searchValue << " appears " << frequencyCount(qThirteenSize, arrayQThirteen, searchValue) << " times in the array.\n";

	cout << endl;
	cout << "Question 14\n";
	const int qFourteenSize = 10;
	int arrayQFourteen[qFourteenSize] = { 11,23,20,37,74,48,35,23,20,37 };
	cout << "The total number of distinct duplicate elements in the array is " << countDuplicates(qFourteenSize, arrayQFourteen) << endl;

	cout << endl;
	cout << "Question 15\n";
	const int qFifteenSize = 4;
	int arrayQFifteen[qFifteenSize] = {1,2,3,4 };
	for (int index = 0; index < qFifteenSize; index++) {
		cout << arrayQFifteen[index] << " ";
	}
	cout << endl;
	reverse(qFifteenSize, arrayQFifteen);

	for (int index = 0; index < qFifteenSize; index++) {
		cout << arrayQFifteen[index] << " ";
	}
	cout << endl;

	cout << endl;
	cout << "Question 16\n";
	const int qSixteenSize = 6;
	int arrayQSixteen[qSixteenSize] = { 1,2,3,4,5,6 };
	for (int index = 0; index < qSixteenSize; index++) {
		cout << arrayQSixteen[index] << " ";
	}
	cout << endl;
	rotateLeft(qSixteenSize, arrayQSixteen);
	for (int index = 0; index < qSixteenSize; index++) {
		cout << arrayQSixteen[index] << " ";
	}
	cout << endl;
	
	cout << endl;
	cout << "Question 17\n";
	const int qSeventeenSize = 7;
	int movies[qSeventeenSize] = { 90, 85, 75, 60, 120, 150, 125 };
	int flight1 = 250;
	if (twoMovies(flight1, movies, qSeventeenSize)) {
		cout << "Two movies fit the flight perfectly!\n";
	}
	else {
		cout << "No perfect pair found.\n";
	}
	int flight2 = 150;
	if (twoMovies(flight2, movies, qSeventeenSize)) {
		cout << "Two movies fit the flight perfectly!\n";
	}
	else {
		cout << "No perfect pair found.\n";
	}

	cout << endl;
	cout << "Question 18\n";
	char text[] = "These are not the 234 droids you were looking for";
	int qEighteenSize = sizeof(text) - 1; 
	cout << "Word count: " << wordCounter(qEighteenSize, text) << endl;

	cout << endl;
	cout << "Question 19\n";
	drawIsocelesTriangle2();

}