


















namespace stagefright {


extern void initialize_string8();
extern void terminate_string8();


extern void initialize_string16();
extern void terminate_string16();

class LibUtilsFirstStatics
{
public:
    LibUtilsFirstStatics()
    {
        initialize_string8();
        initialize_string16();
    }
    
    ~LibUtilsFirstStatics()
    {
        terminate_string16();
        terminate_string8();
    }
};

static LibUtilsFirstStatics gFirstStatics;
int gDarwinCantLoadAllObjects = 1;

}   
