__kernel
void tea(__global unsigned long *inputData,
		 __global unsigned long *outputData,
		 __global unsigned int  *key) {

	int gid = get_global_id(0);
	int gsize = get_global_size(0);
	
	int lid = get_local_id(0);
	int lsize = get_local_size(0);

	//printf("hello");

	//Implement shared mem after - find out how to externally alloc
	// __local int shmem[];
	// lmem[lid] = input[gid];

	//barrier(CLK_LOCAL_MEM_FENCE);
	//if (gid % 2 == 0){ // very inefficient - half threads doing nothing f

	unsigned int threadData[2];
	threadData[0] = (inputData[gid] >> 32) & 0xFFFFFFFF;
	threadData[1] = inputData[gid] & 0xFFFFFFFF;

	unsigned int y = threadData[0];
	unsigned int z = threadData[1];

	unsigned long sum = 0;
	unsigned long delta = 0x9e3779b9; //ARBITRARY, put in shmem
	int n;

	//printf("going into thread encryption v0: %i v1: %i k0: %i k1: %i k2: %i..\n", y, z, key[0], key[1], key[2]);
	for (n = 0; n < 32; n++){
		sum += delta; //(op in shmem?)
		y += ((z << 4) + key[0]) ^ (z + sum) ^ ((z >> 5) + key[1]);
		z += ((y << 4) + key[2]) ^ (y + sum) ^ ((y >> 5) + key[3]);
	}

	outputData[gid] = ((unsigned long)y << 32) + z;

}