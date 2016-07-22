__kernel
void tea (__global long *inputData
		  __global long *outputData
		  __global long *key) {

	int gid = get_global_id(0);
	int gsize = get_global_size(0);
	
	int lid = get_local_id(0);
	int lsize = get_local_size(0);

	//Implement shared mem after - find out how to externally alloc
	// __local int shmem[];
	// lmem[lid] = input[gid];

	//barrier(CLK_LOCAL_MEM_FENCE);

	threadData = inpudData[gid]; // future - use shmem
	int y = (threadData >> 32) & 0xFFFF;
	int z = threadData & 0xFFFF;

	long sum = 0;
	delta = 0x9e3779b9; //put in shmem
	n = 32;

	while (n-- > 0){
		sum += delta; //(op in shmem?)
		y += ((z << 4) + key[0]) ^ (z + sum) ^ ((z >> 5) + k[1]);
		z += ((y << 4) + key[2]) ^ (y + sum) ^ ((y >> 5) + k[3]);
	}

	outputData[gid] = z + (y << 32);
}