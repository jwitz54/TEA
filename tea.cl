__kernel
void tea(__global long *inputData,
		 __global long *outputData,
		 __global long *key) {

	int gid = get_global_id(0);
	int gsize = get_global_size(0);
	
	int lid = get_local_id(0);
	int lsize = get_local_size(0);

	//printf("hello");

	//Implement shared mem after - find out how to externally alloc
	// __local int shmem[];
	// lmem[lid] = input[gid];

	//barrier(CLK_LOCAL_MEM_FENCE);
	//if (gid % 2 == 0){ // very inefficient - half threads doing nothing

	int threadData[2];
	threadData[0] = (inputData[gid] >> 32) & 0xFFFF;
	threadData[1] = inputData[gid] & 0xFFFF;

	int y = threadData[0];
	int z = threadData[1];

	long sum = 0;
	long delta = 0x9e3779b9; //ARBITRARY, put in shmem
	int n = 32;

	if (gid == 0){
		printf("Thread data is: %i , %i \n", y, z);
		printf("Key0 is: %i\n", key[0]);
	}

	while (n-- > 0){
		sum += delta; //(op in shmem?)
		y += ((z << 4) + key[0]) ^ (z + sum) ^ ((z >> 5) + key[1]);
		z += ((y << 4) + key[2]) ^ (y + sum) ^ ((y >> 5) + key[3]);
	}

	outputData[gid] = z + (y << 32);

}