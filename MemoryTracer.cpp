
/*! @file
 *  This is an example of the PIN tool that demonstrates some basic PIN APIs 
 *  and could serve as the starting point for developing your first PIN tool
 */

#include "pin.H"
#include <iostream>
#include <fstream>

/* ================================================================== */
// Global variables 
/* ================================================================== */

UINT64 insCount = 0;        //number of dynamically executed instructions
UINT64 bblCount = 0;        //number of dynamically executed basic blocks
UINT64 threadCount = 0;     //total number of threads, including main thread

std::ostream * out = &cerr;

/* ===================================================================== */
// Command line switches
/* ===================================================================== */
KNOB<string> KnobOutputFile(KNOB_MODE_WRITEONCE,  "pintool",
                            "o", "", "specify file name for MemoryTracer output");

KNOB<BOOL>   KnobCount(KNOB_MODE_WRITEONCE,  "pintool",
                       "count", "1", "count instructions, basic blocks and threads in the application");

KNOB<string> KnobFunction(KNOB_MODE_WRITEONCE,  "pintool",
                          "f", "", "specify the name of function to trace");

string target_func("__NO_SUCH_FUNCTION__");

/* ===================================================================== */
// Utilities
/* ===================================================================== */

/*!
 *  Print out help message.
 */
INT32 Usage()
{
  cerr << "This tool prints out the number of dynamically executed " << endl <<
      "instructions, basic blocks and threads in the application." << endl << endl;

  cerr << KNOB_BASE::StringKnobSummary() << endl;

  return -1;
}

/* ===================================================================== */
// Analysis routines
/* ===================================================================== */

/*!
 * Increase counter of the executed basic blocks and instructions.
 * This function is called for every basic block when it is about to be executed.
 * @param[in]   numInstInBbl    number of instructions in the basic block
 * @note use atomic operations for multi-threaded applications
 */
VOID CountBbl(UINT32 numInstInBbl)
{
  bblCount++;
  insCount += numInstInBbl;
}

// Print a memory read record
VOID RecordMemRead(VOID * ip, VOID * addr, UINT32 size)
{
  *out << ip << ": Reading " << size
       << " bytes of memory at " << addr << endl;  
}

// Print a memory write record
VOID RecordMemWrite(void *ip, VOID * addr, UINT32 size)
{
  *out << ip << ": Writing " << size
       << " bytes of memory at " << addr << endl;  
}


/* ===================================================================== */
// Instrumentation callbacks
/* ===================================================================== */

/*!
 * Insert call to the CountBbl() analysis routine before every basic block 
 * of the trace.
 * This function is called every time a new trace is encountered.
 * @param[in]   trace    trace to be instrumented
 * @param[in]   v        value specified by the tool in the TRACE_AddInstrumentFunction
 *                       function call
 */
VOID Trace(TRACE trace, VOID *v)
{
  RTN rtn = TRACE_Rtn(trace);
  if (!RTN_Valid(rtn)) return;
  
  const string &rtn_name = RTN_Name(rtn);
  
  if (rtn_name != target_func) return;

  *out << "Trace of target function, " << target_func << ", found." << endl;

  // Visit every basic block in the trace
  for (BBL bbl = TRACE_BblHead(trace); BBL_Valid(bbl); bbl = BBL_Next(bbl))
  {
    // Insert a call to CountBbl() before every basic bloc, passing the number of instructions
    BBL_InsertCall(bbl, IPOINT_BEFORE, (AFUNPTR)CountBbl, IARG_UINT32, BBL_NumIns(bbl), IARG_END);

    for (INS ins = BBL_InsHead(bbl); INS_Valid(ins); ins = INS_Next(ins)) {

      if (INS_IsMemoryRead(ins)) {
          INS_InsertPredicatedCall(
              ins, IPOINT_BEFORE, (AFUNPTR)RecordMemRead,
              IARG_INST_PTR,
              IARG_MEMORYREAD_EA,
              IARG_MEMORYREAD_SIZE,
              IARG_END);
      }

      if (INS_HasMemoryRead2(ins)) {
          INS_InsertPredicatedCall(
              ins, IPOINT_BEFORE, (AFUNPTR)RecordMemRead,
              IARG_INST_PTR,
              IARG_MEMORYREAD2_EA,
              IARG_MEMORYREAD_SIZE,
              IARG_END);
      }
      
      if (INS_IsMemoryWrite(ins)) {
          INS_InsertPredicatedCall(
              ins, IPOINT_BEFORE, (AFUNPTR)RecordMemWrite,
              IARG_INST_PTR,              
              IARG_MEMORYWRITE_EA,
              IARG_MEMORYWRITE_SIZE,
              IARG_END);
      }
    }
  }
}

/*!
 * Increase counter of threads in the application.
 * This function is called for every thread created by the application when it is
 * about to start running (including the root thread).
 * @param[in]   threadIndex     ID assigned by PIN to the new thread
 * @param[in]   ctxt            initial register state for the new thread
 * @param[in]   flags           thread creation flags (OS specific)
 * @param[in]   v               value specified by the tool in the 
 *                              PIN_AddThreadStartFunction function call
 */
VOID ThreadStart(THREADID threadIndex, CONTEXT *ctxt, INT32 flags, VOID *v)
{
  threadCount++;
}

/*!
 * Print out analysis results.
 * This function is called when the application exits.
 * @param[in]   code            exit code of the application
 * @param[in]   v               value specified by the tool in the 
 *                              PIN_AddFiniFunction function call
 */
VOID Fini(INT32 code, VOID *v)
{
  *out <<  "===============================================" << endl;
  *out <<  "MemoryTracer analysis results: " << endl;
  *out <<  "Number of instructions: " << insCount  << endl;
  *out <<  "Number of basic blocks: " << bblCount  << endl;
  *out <<  "Number of threads: " << threadCount  << endl;
  *out <<  "===============================================" << endl;
}

/*!
 * The main procedure of the tool.
 * This function is called when the application image is loaded but not yet started.
 * @param[in]   argc            total number of elements in the argv array
 * @param[in]   argv            array of command line arguments, 
 *                              including pin -t <toolname> -- ...
 */
int main(int argc, char *argv[])
{
  // Initialize PIN library. Print help message if -h(elp) is specified
  // in the command line or the command line is invalid 
  if( PIN_Init(argc,argv) )
  {
    return Usage();
  }

  PIN_InitSymbols();
  
  string fileName = KnobOutputFile.Value();

  if (!fileName.empty()) { out = new std::ofstream(fileName.c_str());}
  
  target_func = KnobFunction.Value();
  if (target_func.empty()) {
    return Usage();
  }

  if (KnobCount)
  {
    // Register function to be called to instrument traces
    TRACE_AddInstrumentFunction(Trace, 0);

    // Register function to be called for every thread before it starts running
    PIN_AddThreadStartFunction(ThreadStart, 0);

    // Register function to be called when the application exits
    PIN_AddFiniFunction(Fini, 0);
  }
    
  cerr <<  "===============================================" << endl;
  cerr <<  "Function " << target_func << " is instrumented by Memory Tracer" << endl;  
  if (!KnobOutputFile.Value().empty()) 
  {
    cerr << "See file " << KnobOutputFile.Value() << " for analysis results" << endl;
  }
  cerr <<  "===============================================" << endl;

  // Start the program, never returns
  PIN_StartProgram();
    
  return 0;
}

/* ===================================================================== */
/* eof */
/* ===================================================================== */
