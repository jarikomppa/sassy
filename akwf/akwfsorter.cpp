#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <io.h>

short *data;
int count;
int *lively;
int *order;

void loadfiles()
{
    _finddata_t fileinfo;
    intptr_t handle = _findfirst("*.wav", &fileinfo);
    if (handle == -1)
    {
        printf("No waves found\n");
        return;
    }
    count = 0;
    do
    {
        printf("%s    \r", fileinfo.name);
        count++;
    }
    while (_findnext(handle, &fileinfo) == 0);
    _findclose(handle);

    printf("%d waves found             \n", count);
    data = new short[count * 600];

    handle = _findfirst("*.wav", &fileinfo);
    if (handle == -1)
    {
        printf("No waves found?!\n");
        return;
    }
    count = 0;
    do
    {
        printf("%s    \r", fileinfo.name);
        FILE * f = fopen(fileinfo.name, "rb");
        fseek(f, 44, SEEK_SET);
        fread(data + count * 600, 600, 2, f);
        fclose(f);
        count++;
    }
    while (_findnext(handle, &fileinfo) == 0);
    _findclose(handle);
    
    printf("%d bytes loaded        \n", count * 1200);
}

int liveliness_sort(const void *a, const void *b)
{
    return lively[*(int*)a] - lively[*(int*)b];
}

void liveliness()
{
    printf("Estimating liveliness..\n");
    lively = new int[count];    
    for (int i = 0; i < count; i++)
    {
        int deltasum = 0;
        for (int j = 1; j < 600; j++)
        {
            int delta = data[i*600+j]-data[i*600+j-1];
            deltasum += abs(delta);
        }
        lively[i] = deltasum;
    }
    
    order = new int[count];
    printf("Sorting..\n");
    for (int i = 0; i < count; i++)
        order[i] = i;
        
    //qsort(order, count, sizeof(int), liveliness_sort);
    
    printf("deltas range from %d to %d\n", lively[order[0]], lively[order[count-1]]);        
}

void analyze(int v, int &dc, int &ofs)
{
    int min = 1000000, max = -1000000;
    for (int i = 0; i < 600; i++)
    {
        if (data[v * 600 + i] < min) min = data[v * 600 + i];
        if (data[v * 600 + i] > max) max = data[v * 600 + i];
    }
    dc = (min+max) / 2;
    ofs = 0;
    for (ofs = 1; ofs < 600; ofs++)
    {
        if (data[v * 600 + ofs - 1] - dc < 0 &&
            data[v * 600 + ofs] - dc > 0)
            return;
    }
}

int *dc, *ofs;

void analyze_all()
{
    printf("analyzing dc and offset\n");
    dc = new int[count];
    ofs = new int[count];
    for (int i = 0; i < count; i++)
        analyze(i, dc[i], ofs[i]);
}

int compare(int a, int b)
{
    int delta = 0;
    for (int i = 0; i < 600; i++)
    {
        //delta += abs((data[a * 600 + (i + ofs[a]) % 600] - dc[a]) - (data[b * 600 + (i + ofs[b]) % 600] - dc[b]));
        delta += abs((data[a * 600 + (i + ofs[a]) % 600]) - (data[b * 600 + (i + ofs[b]) % 600]));
        //delta += abs(data[a * 600 + i]) - (data[b * 600 + i]);
    }    
    return delta;
}

int *used;

void buddies()
{    
    printf("Finding buddies..\n");
    used = new int[count];
    memset(used, 0, sizeof(int)*count);
    
    FILE * f = fopen("akwf_sorted.raw", "wb");
    
    fwrite(data + order[0] * 600, 600, 2, f);
    
    int dupes = 0;
    used[order[0]] = 1;
    int a = 0;
    for (int i = 0; i < count; i++)
    {
        int closest = 0;
        int value = 10000000000000;
        for (int j = 0; j < count; j++)
        {
            if (!used[order[j]])
            {
                printf("%d vs %d  \r", order[a], order[j]);
                int v = compare(order[a], order[j]);
                if (v < value)
                {
                    value = v;
                    closest = j;
                }
            }
        }
        printf("%d and %d (%3.3f)  \n", order[a], order[closest], value / (32768.0 * 600.0));
        used[order[closest]] = 1;            
        if (value == 0)
        {
            dupes++;
        }
        else
        {
            short temp[600];
            for (int i = 0; i < 600; i++)
                temp[i] = data[order[closest] * 600 + (i + ofs[order[closest]]) % 600];
            fwrite(temp, 600, 2, f);
            a = closest;
        }
    }
    printf("%d duplicates found\n", dupes);
    printf("%d bytes written\n", ftell(f));
    fclose(f);
}


int main(int parc, char ** pars)
{
    loadfiles();
    liveliness();
    analyze_all();
    buddies();
    
    return 0;
}
