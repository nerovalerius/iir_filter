// Write a C program filtsos which reads raw sample data blockwise from a stereo WAV file
// and applies anexported SOS filter structure.
// The IIR coefficients should be included as a header file within filtsos.c:

// Note that the outputs from MATLABand C are not bit - exact, as MATLAB :
// internally still works with double precision floating point even if the filter is converted to single precision floating point
// does not scale apply the gain values before each section, but rather applies a single scalar value
// The maximum amplitude error between MATLABand C differs only in the LSB which is also the achievable goal for your implementation.

#include <stdio.h>
#include <stdint.h>
#include "fdacoefs.h"
// Use the fdacoefs.h file, that is provided within the eLearning course. This file includes
// b_len, a_len;
// MWSPT_NSEC
// float[][] b, a;



// Read raw sample data blockwise from a stereo WAV file


// Apply an Second Order IIR staged filter (SOS)
// Normal IIR Filter - Page 104
// Transform the IIR Filter to a series of second order Filters (SOS) - Page 107
// a series of second order systems(SOS) Example: 2 * 3 = 6 order system

// how to use the coefficients ?
// each b_0, b_1, b_3
// NSEC = 7 are 7 Filters
// b_len = how many coefficients per array position
// float b and b has 3 coefficients per position

// Put the filters together
// Signal Input -> (First Filter) -> Signal Output -> (Second Filter) -> Signal Output -> (Third Filter) -> Final Signal Output
// Take the  audio input x_k and process it with the 1t unit, pass to the next unit and pass to the last unit
// Design one filter unit and then give 3 different parameters to it
// Do not simply add the a part from the fir filter


/* Date  : 2018-06-04
 * Author: Peter Ott
 * Course: AISE-M/ITS-M/ITSB-M DSP1
 * Description:
 * 	Example implementation of 32-bit binary floating point FIR algorithm.
 * 	For WAV files with 16-bit signed stereo data @ 44.1kHz sample rate.
 * Code based on following references:
 * [1]"Tutorial: Decoding Audio (Windows)", msdn.microsoft.com, 2018. [Online]. Available: https://msdn.microsoft.com/en-us/library/windows/desktop/dd757929(v=vs.85).aspx. [Accessed: 23-03-2018].
 */

typedef struct
{
	char RiffSignature[4]; // Should be "RIFF"
	uint32_t FileLengthMinus8Byte; // Should be WAV file length minus 8 Byte
	char WaveSignature[4]; // Should be "WAVE"
} RiffHeader;

typedef struct
{
	char fmtSignature[4]; // Should be "fmt " (including space)
	uint32_t RemainingHeaderLength; // Should be 16 Byte
	uint16_t fmtTag; // e.g. 0x01 for PCM
	uint16_t NumChannels; // e.g. 2 for Stereo
	uint32_t SamplingRate; // e.g. 44100 Hz
	uint32_t BytesPerSecond; // e.g. 176400 (= 44100 * 4) Bytes per Second for Stereo 16-bit PCM???
	uint16_t BlockAlign; // e.g. 4 Bytes for Stereo 16-bit PCM
	uint16_t BitsPerSample; // e.g. 16 for a 16-bit PCM channel
} FmtHeader;

typedef struct
{
	char DataSignature[4]; // Should be "data"
	uint32_t DataLength; // Maximum: File length - 44
} DataHeader;

typedef struct
{
	RiffHeader riffHeader;
	FmtHeader fmtHeader;
	DataHeader dataHeader;
} WaveHeader;

int16_t int16_sat(float x)
{
	if (x > 32767.0f)
		return 0x7FFF;

	if (x < -32768.0f)
		return -0x8000;

	return (int16_t)x;
}

//  int16_t f32_fir(int16_t x, int16_t* pShiftRegister)
//  {
//  	float sum = 0;
//  	for (int i = f32_fir_coeffs_len - 1; i > 0; i--)
//  	{
//  		pShiftRegister[i] = pShiftRegister[i - 1];
//  		sum = ((float)pShiftRegister[i] / 32768.0f * f32_fir_coeffs[i]) + sum;
//  	}
//  	pShiftRegister[0] = x;
//  	sum = ((float)pShiftRegister[0] / 32768.0f * f32_fir_coeffs[0]) + sum;
//  
//  	return int16_sat(sum * 32768);
//  }


int16_t f32_iir(int16_t x, int16_t* pShiftRegister, size_t stage){

	// Buffer
	float w[3] = { 0, 0, 0 };
	float y = 0;

	w[0] = x;

	for (size_t i = 0; i <= b_len; i++) {
		w[0] += (a[stage][i] * w[i+1]);
	}

	for (size_t i = 0; i <= b_len; i++) {
		y += (b[stage][i] * w[i]);
	}

	// w[0] = x + (a[stage][0] * w[1]) + (a[stage][1] * w[2]);
	// y = (b[stage][0] * w[0]) + (b[stage][1] * w[1]) + (b[stage][2] * w[2]);
	//w[2] = w[1];
	//w[1] = w[0];

	// Prepare next Filter Stage
	stage++;

	return int16_stat(y * 32768);
}


int main(int argc, char* argv[])
{
	if (argc < 3)
	{
		printf("CLI call: wav input.wav output.wav\r\n");
		return 0;
	}

	FILE* inputFile = fopen(argv[1], "rb");
	FILE* outputFile = fopen(argv[2], "wb");
	WaveHeader waveHeader;

	// Skip wave header till we know how much bytes are written
	fseek(outputFile, sizeof(WaveHeader), 0);
	fread(&waveHeader, sizeof(WaveHeader), 1, inputFile);

	int16_t buffer[2048], leftSample[1024], rightSample[1024], leftFiltered[1024], rightFiltered[1024];

	int16_t shiftregister_left[f32_fir_coeffs_len] = { 0 }; // C syntax -> {} for C++
	int16_t shiftregister_right[f32_fir_coeffs_len] = { 0 }; // C syntax -> {} for C++

	// Loop over all samples inside the WAV File
	for (uint32_t blockCount = 0; blockCount < waveHeader.dataHeader.DataLength; blockCount += sizeof(int16_t) * 2048)
	{
		fread(&buffer, sizeof(int16_t), 2048, inputFile);

		// Deinterleave
		for (uint16_t i = 0; i < 1024; i++)
		{
			leftSample[i] = buffer[2 * i];
			rightSample[i] = buffer[2 * i + 1];
		}

		// Processing
		for (uint16_t i = 0; i < 1024; i++)
		{


			size_t stage = 0;

			// Stage 1
			leftFiltered[i] = f32_iir(leftSample[i], shiftregister_left, stage);
			rightFiltered[i] = f32_iir(leftSample[i], shiftregister_left, stage);

			// Stage 2
			leftFiltered[i] = f32_iir(leftSample[i], shiftregister_left, stage);
			rightFiltered[i] = f32_iir(leftSample[i], shiftregister_left, stage);

			// Stage 3
			leftFiltered[i] = f32_iir(leftSample[i], shiftregister_left, stage);
			rightFiltered[i] = f32_iir(leftSample[i], shiftregister_left, stage);

			//leftFiltered[i] = f32_fir(leftSample[i], shiftregister_left);
			//rightFiltered[i] = f32_fir(rightSample[i], shiftregister_right);
			//leftFiltered[i]  = leftSample[i];
			//rightFiltered[i] = rightSample[i];
		}

		// Interleave
		for (uint16_t i = 0; i < 1024; i++)
		{
			buffer[2 * i] = leftFiltered[i];
			buffer[2 * i + 1] = rightFiltered[i];
		}

		fwrite(&buffer, sizeof(int16_t), 2048, outputFile);
	}

	// Go to beginning of output file and write updated wave header
	fseek(outputFile, 0, 0);

	// Write updated wave header
	// TODO: Modify waveHeader accordingly
	fwrite(&waveHeader, sizeof(WaveHeader), 1, outputFile);

	fclose(outputFile);
	fclose(inputFile);

	return 0;
}


