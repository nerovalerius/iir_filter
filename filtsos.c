// This Program is built upon the Example implementation of 32-bit binary floating point FIR algorithm from Peter Ott

/* Date  : 2018-06-04
 * Author: Armin Niedermüller
 * Course: DSP 2- Lab
 * Description: Second Order Staged IIR Filter.
 */

#include <stdio.h>
#include <stdint.h>
#include "fdacoefs.h"
#define MAX_ORDER 3
#define REGISTER_SIZE 100

double register_x1[REGISTER_SIZE], register_x2[REGISTER_SIZE], register_y1[REGISTER_SIZE], register_y2[REGISTER_SIZE];

/* Begin - Reference - Parts by Peter Ott */
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
/* End - Reference - Parts by Peter Ott */

// IRR Filter - Direct Form 1
float iir_df_1(int stage, float x)
{
	float y, summation_point;

	summation_point = x * b[stage][0] + b[stage][1] * register_x1[stage] + b[stage][2] * register_x2[stage];
	y = a[stage][0] * summation_point - a[stage][1] * register_y1[stage] - a[stage][2] * register_y2[stage];

	register_x2[stage] = register_x1[stage];
	register_x1[stage] = x;
	register_y2[stage] = register_y1[stage];
	register_y1[stage] = y;

	return int16_sat(y);
}

// IRR Filter - Direct Form 2
int16_t iir_df_2(int16_t x, size_t stage, float* w) {

	float y = 0;
	w[0] = (float)x;
	
	for (size_t i = 0; i < a_len[stage]; i++) {
		w[0] -= (a[stage][i] * w[i + 1]);
	}
	
	for (size_t i = 0; i < b_len[stage]; i++) {
		y += (b[stage][i] * w[i]);
	}
	
	for (int i = MAX_ORDER; i > 0; i--) {
		w[i] = w[i-1]; // Store values for later use of "past" values (e.g x_(k-1))
	}

	return int16_sat(y);
}

/* Begin - Reference - Parts by Peter Ott */
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
	/* End - Reference - Parts by Peter Ott */
	/* Begin - Reference - Parts by Peter Ott */
	int16_t y_left, y_right, x_left, x_right;
	/* End - Reference - Parts by Peter Ott */

	float w[MAX_ORDER+1] = { 0 };	// IIR Buffer
	double y;
	int j, k;

	for (j = 0; j < REGISTER_SIZE; j++) // Init the shift registers.
	{
		register_x1[j] = 0.0;
		register_x2[j] = 0.0;
		register_y1[j] = 0.0;
		register_y2[j] = 0.0;
	}

	/* Begin - Reference - Parts by Peter Ott */
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
		/* End - Reference - Parts by Peter Ott */

		// Processing
		for (uint16_t i = 0; i < 1024; i++)
		{

			// Filter Stage 0 - Direct Form 1
			y_left = iir_df_1(0, leftSample[i]);
			y_right = iir_df_1(0, rightSample[i]);

			// Filter Stage 0 - Direct Form 2
			//y_left = iir_df_2(leftSample[i], 0, w);
			//y_right = iir_df_2(rightSample[i], 0, w);
						
			// Further Filter Stages
			for (size_t stage = 1; stage < MWSPT_NSEC; stage++) {

				x_left = y_left;
				x_right = y_right;

				// Single Stage - Direct Form 1
				y_left = iir_df_1(stage, x_left);
				y_right = iir_df_1(stage, x_right);

				// Single Stage - Direct Form 2
				//y_left = iir_df_2(x_left, stage, w);
				//y_right = iir_df_2(x_right, stage, w);
				//y_right = iir_df_2(x_right, stage, w);

			}

			leftFiltered[i] = y_left;
			rightFiltered[i] = y_right;
		}

		/* Begin - Reference - Parts by Peter Ott */
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
/* End - Reference - Parts by Peter Ott */


