
/*! @file
 *  This is an example of the PIN tool that demonstrates some basic PIN APIs 
 *  and could serve as the starting point for developing your first PIN tool
 */

#include "pin.H"
#include "portability.H"
#include <iostream>
#include <fstream>
#include <stdlib.h>
#include <stddef.h>

/* ================================================================== */
// Global variables 
/* ================================================================== */


/*
 * The ID of the buffer
 */
BUFFER_ID bufId;

/*
 * Thread specific data
 */
TLS_KEY mlog_key;

/*
 * Number of OS pages for the buffer
 */
#define NUM_BUF_PAGES 1024


std::ostream * out = &cerr;

/* ===================================================================== */
// Command line switches
/* ===================================================================== */
KNOB<string> KnobOutputFile(KNOB_MODE_WRITEONCE,  "pintool",
                            "o", "pin.out", "specify file name for MemoryTracer output");

KNOB<string> KnobFunction(KNOB_MODE_WRITEONCE,  "pintool",
                          "f", "__NO_SUCH_FUNCTION__", "specify the name of function to trace");

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

enum {
  TRACE_READ,
  TRACE_WRITE,
  TRACE_FUNC_BEGIN,
  TRACE_FUNC_END
};

struct MEMREF {
  ADDRINT addr;
  UINT32 type;
  UINT32 size;
};

#if 0
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
#endif

/*
 * MLOG - thread specific data that is not handled by the buffering API.
 */
class MLOG
{
  public:
    MLOG(THREADID tid);
    ~MLOG();

    VOID DumpBufferToFile( struct MEMREF * reference, UINT64 numElements, THREADID tid );

  private:
    ofstream _ofile;
};


MLOG::MLOG(THREADID tid)
{
  string filename = KnobOutputFile.Value() + "." + decstr(getpid_portable()) + "." + decstr(tid);
  cerr << "New MLOG: Trace will be saved in " << filename << endl;
  _ofile.open(filename.c_str());
    
  if ( ! _ofile )
  {
    cerr << "Error: could not open output file." << endl;
    exit(1);
  }
    
  _ofile << hex;
}


MLOG::~MLOG()
{
    _ofile.close();
}


VOID MLOG::DumpBufferToFile( struct MEMREF * reference, UINT64 numElements, THREADID tid )
{
    for(UINT64 i=0; i<numElements; i++, reference++)
    {
      //if (reference->addr != 0)
      _ofile << reference->type << " " << reference->addr
             << " " << reference->size << endl;
    }
}


/* ===================================================================== */
// Instrumentation callbacks
/* ===================================================================== */
#if 0
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

  //*out << "Rtn name: " << rtn_name << endl;
  
  if (rtn_name != target_func) return;
  
  LOG("Trace of target function, " + target_func + ", found." + endl);

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
#endif

/*
 * Insert code to write data to a thread-specific buffer for instructions
 * that access memory.
 */
VOID Trace(TRACE trace, VOID *v)
{
  RTN rtn = TRACE_Rtn(trace);
  if (!RTN_Valid(rtn)) return;
  
  const string &rtn_name = RTN_Name(rtn);

  //*out << "Rtn name: " << rtn_name << endl;
  
  if (rtn_name != target_func) return;
  
  cerr << "Trace of target function, " << target_func << ", found.\n";
  
  for(BBL bbl = TRACE_BblHead(trace); BBL_Valid(bbl); bbl=BBL_Next(bbl))
  {
    for(INS ins = BBL_InsHead(bbl); INS_Valid(ins); ins=INS_Next(ins))
    {
      UINT32 memoryOperands = INS_MemoryOperandCount(ins);

      for (UINT32 memOp = 0; memOp < memoryOperands; memOp++)
      {
        UINT32 refSize = INS_MemoryOperandSize(ins, memOp);

        // Note that if the operand is both read and written we log it once
        // for each.
        if (INS_MemoryOperandIsRead(ins, memOp))
        {
          INS_InsertFillBufferPredicated(
              ins, IPOINT_BEFORE, bufId,
              IARG_MEMORYOP_EA, memOp, offsetof(struct MEMREF, addr),
              IARG_UINT32, refSize, offsetof(struct MEMREF, size),
              IARG_UINT32, TRACE_READ, offsetof(struct MEMREF, type),
              IARG_END);
        }

        if (INS_MemoryOperandIsWritten(ins, memOp))
        {
          INS_InsertFillBufferPredicated
              (ins, IPOINT_BEFORE, bufId,
               IARG_MEMORYOP_EA, memOp, offsetof(struct MEMREF, addr),
               IARG_UINT32, refSize, offsetof(struct MEMREF, size), 
               IARG_UINT32, TRACE_WRITE, offsetof(struct MEMREF, type),
               IARG_END);
        }
      }
    }
  }
}

/**************************************************************************
 *
 *  Callback Routines
 *
 **************************************************************************/

/*!
 * Called when a buffer fills up, or the thread exits, so we can process it or pass it off
 * as we see fit.
 * @param[in] id		buffer handle
 * @param[in] tid		id of owning thread
 * @param[in] ctxt		application context
 * @param[in] buf		actual pointer to buffer
 * @param[in] numElements	number of records
 * @param[in] v			callback value
 * @return  A pointer to the buffer to resume filling.
 */
VOID * BufferFull(BUFFER_ID id, THREADID tid, const CONTEXT *ctxt, VOID *buf,
                  UINT64 numElements, VOID *v)
{
    struct MEMREF * reference=(struct MEMREF*)buf;

    MLOG * mlog = static_cast<MLOG*>( PIN_GetThreadData( mlog_key, tid ) );

    mlog->DumpBufferToFile( reference, numElements, tid );
    
    return buf;
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
  // There is a new MLOG for every thread.  Opens the output file.
  MLOG * mlog = new MLOG(threadIndex);

  // A thread will need to look up its MLOG, so save pointer in TLS
  PIN_SetThreadData(mlog_key, mlog, threadIndex);
}

/*!
 * Print out analysis results.
 * This function is called when the application exits.
 * @param[in]   code            exit code of the application
 * @param[in]   v               value specified by the tool in the 
 *                              PIN_AddFiniFunction function call
 */
VOID ThreadFini(THREADID tid, const CONTEXT *ctxt, INT32 code, VOID *v)
{
  MLOG * mlog = static_cast<MLOG*>(PIN_GetThreadData(mlog_key, tid));

  delete mlog;

  PIN_SetThreadData(mlog_key, 0, tid);
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

  if (!fileName.empty()) {
    out = new std::ofstream(fileName.c_str());
  } else {
    return Usage();
  }
  
  target_func = KnobFunction.Value();
  if (target_func.empty()) {
    return Usage();
  }

  // Initialize the memory reference buffer;
  // set up the callback to process the buffer.
  //
  bufId = PIN_DefineTraceBuffer(sizeof(struct MEMREF), NUM_BUF_PAGES,
                                BufferFull, 0);

  if(bufId == BUFFER_ID_INVALID)
  {
    cerr << "Error: could not allocate initial buffer" << endl;
    return 1;
  }

  // Initialize thread-specific data not handled by buffering api.
  mlog_key = PIN_CreateThreadDataKey(0);
   

  // Register function to be called to instrument traces
  TRACE_AddInstrumentFunction(Trace, 0);

  // Register function to be called for every thread before it starts running
  PIN_AddThreadStartFunction(ThreadStart, 0);

  // Register function to be called when the application exits
  PIN_AddThreadFiniFunction(ThreadFini, 0);

    
  cerr <<  "===============================================" << endl;
  cerr <<  "Function " << target_func << " is instrumented by Memory Tracer" << endl;  
  cerr <<  "See file " << KnobOutputFile.Value() << ".PID.TID for analysis results" << endl;
  cerr <<  "===============================================" << endl;

  // Start the program, never returns
  PIN_StartProgram();
    
  return 0;
}

/* ===================================================================== */
/* eof */
/* ===================================================================== */
