#include <stdio.h>
#include <stdint.h>
#include <CL/cl.h>

#include "utils.c"

//Fcns for reference calc - see botton
void encrypt (uint32_t* v, uint32_t* k, int data_size);
void decrypt (uint32_t* v, uint32_t* k, int data_size);

int main(int argc, char**argv){
	/*Get file info*/
	FILE *inputFile;
	inputFile = fopen("jpeg-pic.jpg", "r+");
	if (inputFile == NULL){
		printf("Error: Could not open file\n");
		fclose(inputFile);
		return 1;
	}
	int num = fseek(inputFile, 0, SEEK_END); //point to end of file
	int file_bytes = ftell(inputFile);
	num = fseek(inputFile, 0, SEEK_SET); //point to beginning of file
	
	int longsInFile = file_bytes/(sizeof(unsigned long));

	/*Declare Constants Based on File*/
	const int data_size = longsInFile;
	const int data_bytes = file_bytes;
	const int work_size = 5; //Due to change for optimization
	const int key_size = 4;
	const int key_bytes = key_size * sizeof(int);

	/*Fill Input Data*/
	unsigned long *hInputData = (unsigned long*)malloc(data_bytes);
	int i;
	for(i = 0; i < data_size; i++){
		unsigned long buf;
		fread(&buf, sizeof(long), 1, inputFile);
		hInputData[i] = buf;
	}
	fclose(inputFile);

	printf("hin0: %lx\n", (unsigned long)hInputData[0]);

	/*Malloc output data*/
	unsigned long *hOutputData = (unsigned long*)malloc(data_bytes);

	/*Write Key*/
	int *hKey = (int*)malloc(key_bytes); //may need to be bigger!
	hKey[0] = 322324;
	hKey[1] = 65645;
	hKey[2] = 323213;
	hKey[3] = 54543;


	/*Perform Reference Calc*/
	//Copy input data
	uint32_t *refData = (uint32_t*)malloc(data_bytes * 2); //Use * 2 because splitting into uints
	uint32_t *refKey = (uint32_t*)malloc(key_bytes);
	refKey[0] = hKey[0];
	refKey[1] = hKey[1];
	refKey[2] = hKey[2];
	refKey[3] = hKey[3];

	for (i = 0; i < data_size; i++){
		refData[2 * i] = (hInputData[i] >> 32) & 0xFFFFFFFF;
		refData[2 * i + 1] = hInputData[i] & 0xFFFFFFFF; 
	}

	//printf("hinputData1: %i refData2: %i refData3 %i\n", hInputData[1], refData[2], refData[3]);

	encrypt(refData, refKey, data_size);

	/*Status variable for checks*/
	cl_int status;

	/*Find Platforms*/
	cl_platform_id platform;
	status = clGetPlatformIDs(1, &platform, NULL);
	check(status);

	/*Find Device*/
	cl_device_id device;
	status = clGetDeviceIDs(platform, CL_DEVICE_TYPE_ALL, 1, &device, NULL); // GPU if on gpu
	check(status);

	/*Figure out max group size*/
	// char buffer[10240];
	// status = clGetDeviceInfo(device, CL_DEVICE_MAX_WORK_GROUP_SIZE, buffer, sizeof(buffer), NULL);
	// check(status);
	// printf("max work: %s \n\n", buffer);

	/*Create Context*/
	cl_context context;
	context = clCreateContext(NULL, 1, &device, NULL, NULL, &status);
	check(status);

	/*Create Command Queue*/
	cl_command_queue cmdQueue;
	cmdQueue = clCreateCommandQueueWithProperties(context, device, NULL, &status);
	check(status);

	/*Create buffer for input data*/
	cl_mem bufInputData;
	bufInputData = clCreateBuffer(context, CL_MEM_READ_ONLY, data_bytes, NULL, &status);
	check(status);

	/*Create buffer for output data*/
	cl_mem bufOutputData;
	bufOutputData = clCreateBuffer(context, CL_MEM_WRITE_ONLY, data_bytes, NULL, &status);
	check(status);

	/*Create key buffer*/
	cl_mem bufKey;
	bufKey = clCreateBuffer(context, CL_MEM_READ_ONLY, key_bytes, NULL, &status);
	check(status);

	/*Write input data to device*/
	status = clEnqueueWriteBuffer(cmdQueue, bufInputData, CL_TRUE, 0, data_bytes, hInputData, 0, NULL, NULL);
	check(status);

	/*Initialize output*/
	// unsigned long zero = 1;
	// status = clEnqueueFillBuffer(cmdQueue, bufOutputData, &zero, sizeof(unsigned long), 0, data_bytes, 0, NULL, NULL);
	// check(status);

	/*Write key to device*/
	status = clEnqueueWriteBuffer(cmdQueue, bufKey, CL_TRUE, 0, key_bytes, hKey, 0, NULL, NULL);
	check(status);

	/*Create Prog*/
	char *programSource = readFile("tea.cl");
	size_t programSourceLen = strlen(programSource);
	cl_program program = clCreateProgramWithSource(context, 1, (const char**)&programSource, &programSourceLen, &status);
	check(status);

	status = clBuildProgram(program, 1, &device, NULL, NULL, NULL);
	if (status != CL_SUCCESS){
		printCompilerError(program, device);
		exit(-1);
	}

	/*Create Kernel*/
	cl_kernel kernel; 
	kernel = clCreateKernel(program, "tea", &status);
	check(status);

	/*Set kernel arguments (in, out, num)*/
	//int elements = DATA_SIZE;

	status  = clSetKernelArg(kernel, 0, sizeof(cl_mem), &bufInputData);
	status |= clSetKernelArg(kernel, 1, sizeof(cl_mem), &bufOutputData);
	status |= clSetKernelArg(kernel, 2, sizeof(cl_mem), &bufKey);  
	check(status);

	/*Define global and work group size*/
	size_t globalWorkSize[1];
	globalWorkSize[0] = data_size;

	size_t localWorkSize[1];
	localWorkSize[0] = 1;

	/*Enqueue Kernel*/
	status = clEnqueueNDRangeKernel(cmdQueue, kernel, 1, NULL, globalWorkSize, localWorkSize, 0, NULL, NULL);
	check(status);

	/*Read output*/
	status = clEnqueueReadBuffer(cmdQueue, bufOutputData, CL_TRUE, 0, data_bytes, hOutputData, 0, NULL, NULL);
	check(status);

	/*Check encryption with ref calc*/
	int mismatch = 0;
	for (i = 0; i < data_size; i++){
		unsigned long firstData = (unsigned long)refData[2*i] << 32;
		unsigned long refDataC = firstData + refData[2*i + 1];
		if(i == 0){
			printf("henc0: %lx rd1: %lx rd2: %x\n", refDataC, firstData, refData[2*i + 1]);
		}
		if (hOutputData[i] != refDataC){
			if (mismatch == 0){
				//printf("First mismatch at %i, ref: %lu calc %lu\n", i, refDataC, hOutputData[i] );
			}
			mismatch = 1;
		}
	}

	if (mismatch == 0){
		printf("Encryption Succesful! Writing Encrypted to encrypted.bin\n");
		FILE *outFile;
		outFile = fopen("encrpyted.bin", "w+");
		for(i = 0; i < data_size; i++){
			unsigned long num[1];
			num[0] = hOutputData[i];
			fwrite(num, sizeof(unsigned long), 1, outFile);
		}
		fclose(outFile);
	} else {
		printf("Encryption Failed, See Above Mismatch\n");
	}

	/*Decrypt file*/
	uint32_t *refDecryptData = (uint32_t*)malloc(data_bytes * 2); //Use * 2 because splitting into uints
	for (i = 0; i < data_size; i++){
		refDecryptData[2 * i] = (hOutputData[i] >> 32) & 0xFFFFFFFF;
		refDecryptData[2 * i + 1] = hOutputData[i] & 0xFFFFFFFF; 
		if (i == 1){
			//printf("OD: %i refDecryptData1: %i , refDD2: %i\n", hOutputData[i], refDecryptData[2 * i], refDecryptData[2 * i + 1]);
		}
	}
	decrypt(refDecryptData, refKey, data_size);

	/*Check decryption with ref calc*/
	mismatch = 0;
	for (i = 0; i < data_size; i++){
		unsigned long firstDecData = (unsigned long)refDecryptData[2*i] << 32;
		unsigned long refDecryptDataC = firstDecData + refDecryptData[2*i + 1];
		if (i == 0) {
			printf("ref: %lx dec: %lx\n", hInputData[i], refDecryptDataC);
		}
		if (hInputData[i] != refDecryptDataC){
			if (mismatch == 0){
				printf("First decrypt mismatch at %i, ref: %lu calc %lu\n", i, refDecryptDataC, hInputData[i]);
				mismatch = 2;
			} else if (mismatch == 2){
				printf("Second decrypt mismatch at %i, ref: %lu calc %lu\n", i, refDecryptDataC, hInputData[i]);
				mismatch = 1;
			}
			//mismatch = 1;
		}
	} 
	if (mismatch == 0){
		printf("Decryption Succesful! Writing Decrypted to decrypted.jpg\n");
		FILE *outDecFile;
		outDecFile = fopen("decrypted.jpg", "w+");
		for(i = 0; i < data_size; i++){
			unsigned long num[1];
			num[0] = hOutputData[i];
			fwrite(num, sizeof(unsigned long), 1, outDecFile);
		}
		fclose(outDecFile);
	} else {
		printf("Decryption Failed, See Above Mismatch\n");
	}

	return (0);
}

//REFERENCE CALCS TAKEN + MODIFIED FROM https://en.wikipedia.org/wiki/Tiny_Encryption_Algorithm
void encrypt (uint32_t* v, uint32_t* k, int data_size) {
	int i;
	for (i = 0; i < data_size*2; i += 2){
	    uint32_t v0=v[i], v1=v[i + 1], sum=0, w;           /* set up */
	    uint32_t delta=0x9e3779b9;                     /* a key schedule constant */
	    uint32_t k0=k[0], k1=k[1], k2=k[2], k3=k[3];   /* cache key */
	    //printf("going into ref encryption v0: %i v1: %i k0: %i k1: %i k2: %i..\n", v0, v1, k0, k1, k2);
	    for (w=0; w < 32; w++) {                       /* basic cycle start */
	        sum += delta;
	        v0 += ((v1<<4) + k0) ^ (v1 + sum) ^ ((v1>>5) + k1);
	        v1 += ((v0<<4) + k2) ^ (v0 + sum) ^ ((v0>>5) + k3);
	    }                                              /* end cycle */
	    v[i]=v0; v[i + 1]=v1;
	}
}

void decrypt (uint32_t* v, uint32_t* k, int data_size) {
	int i;
	for (i = 0; i < data_size*2; i += 2){
	    uint32_t v0=v[i], v1=v[i + 1], sum=0xC6EF3720, w;  /* set up */
	    uint32_t delta=0x9e3779b9;                     /* a key schedule constant */
	    uint32_t k0=k[0], k1=k[1], k2=k[2], k3=k[3];   /* cache key */	
		//if (i == 0 || i == 2){
	    	//printf("going into ref decryption v0: %i v1: %i k0: %i k1: %i k2: %i..\n", v0, v1, k0, k1, k2);
	    //}
	    for (w=0; w<32; w++) {                         /* basic cycle start */
	        v1 -= ((v0<<4) + k2) ^ (v0 + sum) ^ ((v0>>5) + k3);
	        v0 -= ((v1<<4) + k0) ^ (v1 + sum) ^ ((v1>>5) + k1);
	        sum -= delta;
	    }                                              /* end cycle */
	    v[i]=v0; v[i + 1]=v1;
	}
}