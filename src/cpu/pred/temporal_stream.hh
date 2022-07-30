#ifndef __CPU_TEMPORAL_STREAM_PRED_HH__
#define __CPU_TEMPORAL_STREAM_PRED_HH__

#include <bitset>
#include <map>
#include <vector>

#include "base/sat_counter.hh"
#include "base/types.hh"
#include "cpu/pred/bi_mode.hh"
#include "cpu/pred/bpred_unit.hh"
// #include "cpu/pred/ltage.hh"
#include "params/TemporalStreamBP.hh"

#define HTB_INIT 2
#define TS_KEY_SIZE (140 + 64)
// params/TemporalStreamBP.h is auto-generated.
// it should be under build/X86/params

namespace std{
  template<std::size_t N>
  struct bitset_less
  {
    bool operator() (const std::bitset<N>& x, const std::bitset<N>& y) const
    {
      if (N <= 64) return x.to_ulong() < y.to_ulong();

      for (int i = N-1; i >= 0; i--) {
          if (x[i] && !y[i]) return false;
          if (!x[i] && y[i]) return true;
      }
      return false;
    }
  };
}

namespace gem5
{

  namespace branch_prediction
  {

    /**
     * Implements a tournament branch predictor, hopefully identical to the one
     * used in the 21264.  It has a local predictor, which uses a local history
     * table to index into a table of counters, and a global predictor, which
     * uses a global history to index into a table of counters.  A choice
     * predictor chooses between the two.  Both the global history register
     * and the selected local history are speculatively updated.
     */
    class TemporalStreamBP : public BPredUnit
    {
    public:
      /**
       * Default branch predictor constructor.
       */
      TemporalStreamBP(const TemporalStreamBPParams& params);

      /**
       * Masks the given PC address to a length of TS_KEY_SIZE bitset.
       * @param PC The address of the branch.
       * @return The masked bitset.
       */
      std::bitset<TS_KEY_SIZE> ts_idx(Addr PC, ThreadID tid);

      /**
       * Looks up the given address in the branch predictor and returns
       * a true/false value as to whether it is taken.  Also creates a
       * BPHistory object to store any state it will need on squash/update.
       * @param branch_addr The address of the branch to look up.
       * @param bp_history Pointer that will be set to the BPHistory object.
       * @return Whether or not the branch is taken.
       */
      bool lookup(ThreadID tid, Addr branch_addr, void*& bp_history);

      /**
       * Records that there was an unconditional branch, and modifies
       * the bp history to point to an object that has the previous
       * global history stored in it.
       * @param bp_history Pointer that will be set to the BPHistory object.
       */
      void uncondBranch(ThreadID tid, Addr pc, void*& bp_history);
      /**
       * Updates the branch predictor to Not Taken if a BTB entry is
       * invalid or not found.
       * @param branch_addr The address of the branch to look up.
       * @param bp_history Pointer to any bp history state.
       * @return Whether or not the branch is taken.
       */
      void btbUpdate(ThreadID tid, Addr branch_addr, void*& bp_history);
      /**
       * Updates the branch predictor with the actual result of a branch.
       * @param branch_addr The address of the branch to update.
       * @param taken Whether or not the branch was taken.
       * @param bp_history Pointer to the BPHistory object that was created
       * when the branch was predicted.
       * @param squashed is set when this function is called during a squash
       * operation.
       * @param inst Static instruction information
       * @param corrTarget Resolved target of the branch (only needed if
       * squashed)
       */
      void update(ThreadID tid, Addr branch_addr, bool taken, void* bp_history,
        bool squashed, const StaticInstPtr& inst, Addr corrTarget);

      /**
       * Restores the global branch history on a squash.
       * @param bp_history Pointer to the BPHistory object that has the
       * previous global branch history in it.
       */
      void squash(ThreadID tid, void* bp_history);

    private:

      // we don't care what's inside baseHistory
      // since it's casted as void* type.
      // we just pass it along for the base predictor to work.
      // other info are all stored inside TSHistory along with *baseHistory.

      struct TSHistory
      {
        void *baseHistory;
        bool baseOutcome;
        bool tsOutcome;
        bool uncond;
        Addr trigPC;
      };


      // change here if using another base predictor
      BiModeBP* basePredictor;
      // LTAGE* basePredictor;

      // replay flag of the temporal stream BP.
      bool replayFlag;

      // circular buffer maintained in the temporal stream BP,
      // uses bufferHead and bufferTail to indicate the circular head and tail.
      std::vector<char> circularBuffer;

      std::map<
        ThreadID,
        std::map<
        std::bitset<TS_KEY_SIZE>,
        std::vector<char>::iterator,
        std::bitset_less<TS_KEY_SIZE>
      > >headTable;

      std::map<
        ThreadID,
        std::bitset<TS_KEY_SIZE>
      > ts_gh;
      // size_t is too small for these indexes
      // unsigned bufferHead;
      std::vector<char>::iterator bufferHead;

      // unsigned bufferTail;
      std::vector<char>::iterator bufferTail;

      unsigned bufferSize;


    };

  } // namespace branch_prediction
} // namespace gem5

#endif // __CPU_PRED_TOURNAMENT_PRED_HH__
