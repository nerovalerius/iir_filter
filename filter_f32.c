/* Date  : 2018-06-04 
 * Author: Peter Ott 
 * Course: AISE-M/ITS-M/ITSB-M DSP1
 * Description: 
 * 	Example implementation of 32-bit binary floating point FIR algorithm.
 * 	For WAV files with 16-bit signed stereo data @ 44.1kHz sample rate.
 * Code based on following references:
 * [1]"Tutorial: Decoding Audio (Windows)", msdn.microsoft.com, 2018. [Online]. Available: https://msdn.microsoft.com/en-us/library/windows/desktop/dd757929(v=vs.85).aspx. [Accessed: 23-03-2018].
 */ 

#include <stdio.h>
#include <stdint.h>

#include "fdacoefs_float32.h"

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
    if(x > 32767.0f)
        return 0x7FFF;

    if(x < -32768.0f)
        return -0x8000;

    return (int16_t)x;
}

int16_t f32_fir(int16_t x, int16_t* pShiftRegister)
{
	float sum = 0;
	for(int i=f32_fir_coeffs_len-1; i>0; i--)
	{
		pShiftRegister[i] = pShiftRegister[i-1];
		sum = ((float)pShiftRegister[i]/32768.0f * f32_fir_coeffs[i]) + sum;
	}
	pShiftRegister[0] = x;
	sum = ((float)pShiftRegister[0]/32768.0f * f32_fir_coeffs[0]) + sum;

	return int16_sat(sum*32768);
}

int main(int argc, char* argv[])
{
	if(argc < 3)
	{
		printf("CLI call: wav input.wav output.wav\r\n");
		return 0;
	}
	
	FILE* inputFile = fopen(argv[1],"rb");
	FILE* outputFile = fopen(argv[2], "wb");
	WaveHeader waveHeader;
	
	// Skip wave header till we know how much bytes are written
	fseek(outputFile, sizeof(WaveHeader), 0);
	fread(&waveHeader, sizeof(WaveHeader), 1, inputFile);

	int16_t buffer[2048], leftSample[1024], rightSample[1024],leftFiltered[1024], rightFiltered[1024];

	int16_t shiftregister_left[f32_fir_coeffs_len] = {0}; // C syntax -> {} for C++
	int16_t shiftregister_right[f32_fir_coeffs_len] = {0}; // C syntax -> {} for C++

	for (uint32_t blockCount = 0; blockCount < waveHeader.dataHeader.DataLength; blockCount += sizeof(int16_t)*2048)
	{
		fread(&buffer, sizeof(int16_t), 2048, inputFile);
		
		// Deinterleave
		for (uint16_t i = 0; i < 1024; i++)
		{
			leftSample[i]  = buffer[2 * i];
			rightSample[i] = buffer[2 * i + 1];
		}

		// TODO: Processing
			for(uint16_t i=0; i<1024;i++)
			{
				leftFiltered[i] = f32_fir(leftSample[i],shiftregister_left);
				rightFiltered[i] = f32_fir(rightSample[i],shiftregister_right);
				//leftFiltered[i]  = leftSample[i];
				//rightFiltered[i] = rightSample[i];
			}

		// Interleave
		for (uint16_t i = 0; i < 1024; i++)
		{
			buffer[2 * i]     = leftFiltered[i];
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
