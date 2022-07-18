#include "cpu/pred/temporal_stream.hh"

#include <map>

#include "base/bitfield.hh"
#include "base/intmath.hh"

#define HTB_INIT 2
/*
note:
- btbUpdate, update & squash all takes a void* bp_history,
    recast it as TSHistory object, and reads info from it.
- uncondBranch, lookup takes a void* bp_history,
allocates a new TSHistory and save
    it to bp_history.
- since we use a base predictor, we use TSHistory->baseHistory to
    function as bp_history for the base predictor.
- for most of the methods, we simply call the corresponding
    method of the base predictor.
- only for update & lookup, there's additional work
    related with the TemporalStreamBP
- FIXME: not sure whether to handle the TS circular
    buffer in uncondBranch()
*/
namespace gem5
{

    namespace branch_prediction
    {
        TemporalStreamBP::TemporalStreamBP(
            const TemporalStreamBPParams& params
        ):
            // initialted in BranchPredictor.py::TemporalStreamPredictor.
            BPredUnit(params),
            // also change here if want to use another base predictor.
            basePredictor(params.bi_mode_predictor),
            bufferSize(params.circular_buffer_size)
        {
            replayFlag = false;

            bufferHead = 0;

            bufferTail = 0;

            // setup the circular buffer
            circularBuffer.resize(bufferSize);
            for (int i = 0; i < bufferSize; ++i)
                circularBuffer[i] = HTB_INIT;
        }

        void TemporalStreamBP::btbUpdate(
            ThreadID tid,
            Addr branch_addr,
            void*& bp_history
        )
        {
            TSHistory *history = static_cast<TSHistory *>(bp_history);
            void *baseHistory = (history->baseHistory);

            basePredictor->btbUpdate(tid, branch_addr, baseHistory);
        }

        bool TemporalStreamBP::lookup(
            ThreadID tid,
            Addr branch_addr,
            void*& bp_history
        )
        {
            TSHistory *history = new TSHistory;
            void* baseHistory;
            bool baseOutcome = basePredictor->lookup(
                tid, branch_addr, baseHistory
            );
            history->baseHistory = baseHistory;
            bool tsOutcome;

            if (replayFlag && circularBuffer[bufferHead++%bufferSize]==0)
                tsOutcome = !baseOutcome;
            else
                tsOutcome = baseOutcome;

            history->baseOutcome = baseOutcome;
            history->tsOutcome = tsOutcome;
            bp_history = (void *)history;
            return tsOutcome;
        }

        void TemporalStreamBP::uncondBranch(
            ThreadID tid,
            Addr pc,
            void*& bp_history
        )
        {
            TSHistory *history = new TSHistory;
            void *baseHistory = (history->baseHistory);
            basePredictor->uncondBranch(tid, pc, baseHistory);
        }

        void TemporalStreamBP::update(
            ThreadID tid,
            Addr branch_addr,
            bool taken,
            void* bp_history,
            bool squashed,
            const StaticInstPtr& inst,
            Addr corrTarget
        )
        {

            TSHistory *history = static_cast<TSHistory *>(bp_history);
            void *baseHistory = (history->baseHistory);
            assert(baseHistory);

            basePredictor->update(
                tid, branch_addr, taken,
                baseHistory, squashed,
                inst, corrTarget
            );

            circularBuffer[
                ++bufferTail%bufferSize
            ] = (history->baseOutcome==taken);

            if (history->tsOutcome != taken)
                replayFlag = false;
            if (history->baseOutcome != taken) {
                // FIXME: should we concatenate tid with
                // something in the GHR here?
                // in that case might need to change the header:
                // std::map<ThreadID, unsigned> headTable;
                // => std::map<SOME_CONCAT_TYPE, unsigned> headTable;
                auto iter = headTable.find(tid);
                if (
                    (iter!=headTable.end())
                 && (!replayFlag)
                 && (iter->second!=HTB_INIT)
                )
                {
                    bufferHead = iter->second;
                    replayFlag = true;
                }
                headTable[tid] = bufferTail;
            }

            delete history;
        }

        void TemporalStreamBP::squash(
            ThreadID tid,
            void* bp_history
        )
        {
            TSHistory *history = static_cast<TSHistory *>(bp_history);
            void *baseHistory = (history->baseHistory);

            basePredictor->squash(tid, baseHistory);
            delete history;
        }

    } // namespace branch_prediction
} // namespace gem5
