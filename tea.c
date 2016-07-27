#include <stdio.h>
#include <CL/cl.h>

#include "utils.c"

int main(int argc, char**argv){

	const int data_size = 2;
	const int data_bytes = data_size * sizeof(int);
	const int work_size = 5;
	const int key_size = 128;
	const int key_bytes = key_size/8;

	/*Fill data*/
	int *hInputData = (int*)malloc(data_bytes);

	/* Assignments for testing*/
	hInputData[0] = 1;
	hInputData[1] = 1;

	//if adapt to jpeg, fill in input here

	int *hOutputData = (int*)malloc(data_bytes);

	/*Write Key*/
	int *hKey = (int*)malloc(key_bytes);
	hKey = 1;

	/*Status variable for checks*/
	cl_int status;

	/*Find Platforms*/
	cl_platform_id platform;
	status = clGetPlatformIDs(1, &platform, NULL);
	check(status);
	
	/*Find Device*/
	cl_device_id device;
	status = clGetDeviceIDs(platform, CL_DEVICE_TYPE_CPU, 1, &device, NULL); // GPU if on gpu
	check(status);

	/*Create Context*/
	cl_context context;
	context = clCreateContext(NULL, 1, &device, NULL, NULL, &status);
	check(status);

	/*Create Command Queue*/
	cl_command_queue cmdQueue;
	cmdQueue = clCreateCommandQueueWithProperties(context, device, 0, &status);
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

	/*Initialize output histo*/
	int zero = 0;
	status = clEnqueueFillBuffer(cmdQueue, bufOutputData, &zero, sizeof(int), 0, data_bytes, 0, NULL, NULL);
	check(status);

	/*Create Prog*/
	char *programSource = readFile("tea.cl"); //find readfile
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
	status |= clSetKernelArg(kernel, 2, key_bytes, &bufKey);  
	check(status);

	/*Define global and work group size*/
	size_t globalWorkSize[1];
	globalWorkSize[0] = data_size;

	size_t localWorkSize[1];
	localWorkSize[0] = 16;

	/*Enqueue Kernel*/
	status = clEnqueueNDRangeKernel(cmdQueue, kernel, 1, NULL, globalWorkSize, localWorkSize, 0, NULL, NULL);

	/*Read output*/
	status = clEnqueueReadBuffer(cmdQueue, bufOutputData, CL_TRUE, 0, data_bytes, hOutputData, 0, NULL, NULL);
	check(status);

	printf("Result: %i \n", hOutputData[0]);

	return (0);	
}
