//----------------------------------------------------------------------------
/** @file GoSafetySolver.cpp
    See GoSafetySolver.h.
*/
//----------------------------------------------------------------------------

#include "SgSystem.h"
#include "GoSafetySolver.h"

#include "GoBlock.h"
#include "GoChain.h"
#include "GoSafetyUtil.h"
#include "SgConnCompIterator.h"

using GoSafetyUtil::AddToSafe;
using GoSafetyUtil::ExtendedIsTerritory;
using GoSafetyUtil::IsTerritory;

//----------------------------------------------------------------------------

namespace {

const bool DEBUG_MERGE_CHAINS = false;
const bool DEBUG_SAFETY_SOLVER = false;

bool HaveSharedUnsafe(const SgListOf<GoBlock>& list1,
                      const SgListOf<GoBlock>& list2)
{
    for (SgListIteratorOf<GoBlock> it(list1); it; ++it)
        if (! (*it)->IsSafe() && list2.Contains(*it))
            return true;
    return false;
}

} // namespace

void GoSafetySolver::FindHealthy()
{        
    for (SgBWIterator it; it; ++it)
    {
        SgBlackWhite color(*it);
        for (SgListIteratorOf<GoRegion> it(AllRegions(color)); it; ++it)
            (*it)->ComputeFlag(isStatic1Vital);
    }
   
    // used to just call GoStaticSafetySolver::FindHealthy() here, 
    // but that works with GoBlock's and now we use GoChain's.
    // Code is duplicated though. Can maybe use a template function.
   
    for (SgBWIterator it; it; ++it)
    {
        SgBlackWhite color(*it);
        for (SgListIteratorOf<GoRegion> it(AllRegions(color)); it; ++it)
        {
            GoRegion* r = *it;
            for (SgListIteratorOf<GoChain> it2(r->Chains()); it2; ++it2)
            {
                if (RegionHealthyForBlock(*r, **it2)) // virtual call
                    (*it2)->AddHealthy(r);
}   }   }   }

void GoSafetySolver::FindClosure(SgListOf<GoBlock>* blocks) const
{
    SgListOf<GoBlock> toTest(*blocks);
    while (toTest.NonEmpty())
    {
        const GoBlock* b = toTest.Pop();
        for (SgListIteratorOf<GoRegion> it(b->Healthy()); it; ++it)
        {   GoRegion* r = *it;
            for (SgListIteratorOf<GoChain> it(r->Chains()); it; ++it)
            {   GoBlock* b2 = *it;
                if (! blocks->Contains(b2) && b2->ContainsHealthy(r))
                {
                    blocks->Append(b2);
                    toTest.Append(b2);
}   }   }   }   }

void GoSafetySolver::FindTestSets(SgListOf<SgListOf<GoBlock> >* sets,
                                  SgBlackWhite color) const
{
    SG_ASSERT(sets->IsEmpty());
    SgListOf<GoBlock> doneSoFar;
    for (SgListIteratorOf<GoChain> it(AllChains(color)); it; ++it)
    {
        GoBlock* block = *it;
        if (! doneSoFar.Contains(block))
        {
            SgListOf<GoBlock>* blocks = new SgListOf<GoBlock>;
            blocks->Append(block);
            
            FindClosure(blocks);
            doneSoFar.AppendList(*blocks);
            sets->Append(blocks);
        }
    }
}

bool GoSafetySolver::RegionHealthyForBlock(const GoRegion& r,
                                           const GoBlock& b) const
{
    return    GoStaticSafetySolver::RegionHealthyForBlock(r, b)
           || r.GetFlag(isStatic1Vital);
}


void GoSafetySolver::Test2Vital(GoRegion* r, SgBWSet* safe)
{
    if (r->ComputeAndGetFlag(isStatic2v))
        AddToSafe(Board(), r->Points(), r->Color(),
                  safe, "2-vital:", 0, true);
}

void GoSafetySolver::Find2VitalAreas(SgBWSet* safe)
{
    for (SgBWIterator it; it; ++it)
    {
        SgBlackWhite color(*it);
        for (SgListIteratorOf<GoRegion> it(AllRegions(color)); it; ++it)
            if (     (*it)->Points().Disjoint((*safe)[SG_BLACK]) 
                 && (*it)->Points().Disjoint((*safe)[SG_WHITE]))
            {
                Test2Vital(*it, safe);
                safe->AssertDisjoint();
            }
    }
}

void GoSafetySolver::FindSurroundedSafeAreas(SgBWSet* safe,
                                             SgBlackWhite color)
{
    // AR keep this always up to date earlier.
    Regions()->SetSafeFlags(*safe); 
        
    // try to find single region that becomes safe.
    bool change = true;
    while (change)
    {
        change = false;
        SgPointSet anySafe(safe->Both());
        for (SgListIteratorOf<GoRegion> it(AllRegions(color)); it; ++it)
        {
            GoRegion* r = *it;
            if (   ! r->GetFlag(isSafe)
                && r->SomeBlockIsSafe()
                && ! r->Points().Overlaps(anySafe)
                && ExtendedIsTerritory(Board(), Regions(),
                                       r->PointsPlusInteriorBlocks(),
                                       (*safe)[color],
                                       color)
               )
            {
                AddToSafe(Board(), r->Points(), color, safe,
                          "surr-safe-1", 0, true);
                Regions()->SetSafeFlags(*safe); 
                anySafe = safe->Both();
                change = true;
                break;
            }
        }
    }
    FindSurroundedRegionPairs(safe, color);
}

bool GoSafetySolver::FindSafePair(SgBWSet* safe,
                                  SgBlackWhite color,
                                  const SgPointSet& anySafe,
                                  const GoRegion* r1)
{
    for (SgListIteratorOf<GoRegion> it(AllRegions(color)); it; ++it)
    {
        GoRegion* r2 = *it;
        if (   r2 != r1
            && ! r2->Points().Overlaps(anySafe)
            && HaveSharedUnsafe(r1->Blocks(), r2->Blocks())
           )
        {
            const SgPointSet unionSet(r1->Points() | r2->Points());
            if (IsTerritory(Board(), unionSet, (*safe)[color], color))
            {
                AddToSafe(Board(), unionSet, color, safe,
                          "surr-safe-2", 0, true);
                Regions()->SetSafeFlags(*safe); 
                safe->AssertDisjoint();
                return true;
            }
        }
    }
    return false;
}

void GoSafetySolver::FindSurroundedRegionPairs(SgBWSet* safe,
                                               SgBlackWhite color)
{
    bool change = true;
    while (change)
    {
        change = false;
        SgPointSet anySafe(safe->Both());
        for (SgListIteratorOf<GoRegion> it(AllRegions(color)); it; ++it)
        {   GoRegion* r1 = *it;
            if (   ! r1->GetFlag(isSafe)
                && r1->SomeBlockIsSafe()
                && ! r1->Points().Overlaps(anySafe)
                && FindSafePair(safe, color, anySafe, r1)
               )
            {
                change = true;
                break;
            }
        }
    }
}

void GoSafetySolver::FindSafePoints(SgBWSet* safe)
{    
    GoStaticSafetySolver::FindSafePoints(safe);
    safe->AssertDisjoint();

    // find areas big enough for two eyes
    Find2VitalAreas(safe);
    safe->AssertDisjoint();
 
//    GoStaticSafetySolver::FindSafePoints(safe); 
//called again before 030505,
// but now double-counts healthy regions for chains. Must change to
// set a computedHealthy flag
//    safe->AssertDisjoint();

    // find areas small enough that opponent cannot make two eyes
    for (SgBWIterator it; it; ++it)
    {
        FindSurroundedSafeAreas(safe, *it);
        safe->AssertDisjoint();
    }    

    if (DEBUG_SAFETY_SOLVER)
    {    
        const SgPointSet proved = safe->Both();
        int totalRegions = 0;
        int provedRegions = 0;
        int totalBlocks = 0;
        int provedBlocks = 0;  
    
        for (SgBWIterator it; it; ++it)
        {
            SgBlackWhite color(*it);
            for (SgListIteratorOf<GoRegion> it(AllRegions(color)); it; ++it)
            {
                ++totalRegions;
                if ((*it)->Points().SubsetOf(proved))
                    ++provedRegions;
            }
            for (SgListIteratorOf<GoBlock> it(AllBlocks(color)); it; ++it)
            {
                ++totalBlocks;
                if (proved.Overlaps((*it)->Stones()))
                    ++provedBlocks;
            }
        }
        
        SgDebug() << "\n****GoSafetySolver Result****" << "\n"
            << "Total proved points = " << proved.Size() << "\n"
            << "Total regions =  " << totalRegions
            << " Proved regions = " << provedRegions << "\n"
            << "Total blocks =  " << totalBlocks 
            << " Proved blocks = " << provedBlocks << "\n";
    }
}

void GoSafetySolver::Merge(GoChain* c1, GoChain* c2,
                           GoRegion* r, bool bySearch)
{
    SG_ASSERT(! r->GetFlag(usedForMerge));
    r->SetFlag(usedForMerge, true);
    
    GoChainCondition* c = 0;
    if (bySearch)
        c = new GoChainCondition(chainBySearch);
    else
    {
        SgPoint lib1, lib2;
        r->Find2FreeLibs(c1, c2, &lib1, &lib2);
        c = new GoChainCondition(twoLibsInRegion, lib1, lib2);
    }
    
    GoChain* m = new GoChain(c1, c2, c);

    SgBlackWhite color = c1->Color();
    bool found = AllChains(color).Exclude(c1);
    SG_ASSERT(found);
    found = AllChains(color).Exclude(c2);
    SG_ASSERT(found);
    AllChains(color).Include(m);
    SG_ASSERT(AllChains(color).UniqueElements());

    for (SgListIteratorOf<GoRegion> it(AllRegions(color)); it; ++it)
    {
        GoRegion* r = *it;
        bool replace1 = r->ReplaceChain(c1, m);
        bool replace2 = r->ReplaceChain(c2, m);
        if (replace1 || replace2)
        {
            r->ReInitialize();
            r->ComputeFlag(isStatic1Vital);
        }
    }

    if (DEBUG_MERGE_CHAINS)
    {
        SgDebug() << "\nmerge:";
        c1->WriteID(SgDebug());
        SgDebug() << " + ";
        c2->WriteID(SgDebug());
        SgDebug() << " = ";
        m->WriteID(SgDebug());
        SgDebug() << '\n';
    }

    delete c1;
    delete c2;
}

void GoSafetySolver::GenBlocksRegions()
{
    if (UpToDate())
        /* */ return; /* */
        
    GoStaticSafetySolver::GenBlocksRegions();
    
    Regions()->GenChains();
    
    // merge blocks adjacent to 1-vital with 2 conn. points
    for (SgBWIterator it; it; ++it)
    {
        SgBlackWhite color(*it);
        for (SgListIteratorOf<GoRegion> it(AllRegions(color)); it; ++it)
        {
            GoRegion* r = *it;
            r->ComputeFlag(isStatic1Vital);
        }

        bool changed = true;
        while (changed)
        {   changed = false;
            for (SgListIteratorOf<GoRegion> it(AllRegions(color)); it; ++it)
            {   GoRegion* r = *it;
                if (   r->GetFlag(isStatic1vc)
                    && r->Chains().IsLength(2)
                    && r->Has2Conn() 
                        //  || r->Safe2Cuts(Board()) changed from && to ||
                        // @todo does not work if blocks are already chains???
                        // must explicitly keep chain libs info.
                   )
                // easy case of only 2 chains
                {
                    GoChain* c1 = r->Chains().Top();
                    GoChain* c2 = r->Chains().Tail();
                    Merge(c1, c2, r, false); // false = not by search
                    changed = true;
                    break; // to leave iteration
                }
                else if (   r->GetFlag(isStatic1Vital)
                         && r->GetFlag(isCorridor)
                         && ! r->GetFlag(usedForMerge)
                        ) 
                {
                    GoChain* c1 = 0;
                    GoChain* c2 = 0;
                    if (r->Find2Mergable(&c1, &c2))
                    {
                        Merge(c1, c2, r, false);
                        changed = true;
                        break; // to leave iteration
    }   }   }   }   }
    
    m_code = Board().GetHashCode();
}

