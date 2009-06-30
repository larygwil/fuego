//----------------------------------------------------------------------------
/** @file GoStaticSafetySolver.cpp
    See GoStaticSafetySolver.h.
*/
//----------------------------------------------------------------------------

#include "SgSystem.h"
#include "GoStaticSafetySolver.h"

#include "GoBlock.h"
#include "GoChain.h"
#include "GoSafetyUtil.h"
#include "SgDebug.h"

using GoSafetyUtil::AddToSafe;

//----------------------------------------------------------------------------

GoStaticSafetySolver::GoStaticSafetySolver(const GoBoard& board,
                                           GoRegionBoard* regions)
    : m_board(board),
      m_allocRegion(! regions)
{
    if (regions)
        m_regions = regions;
    else
        m_regions = new GoRegionBoard(board);
}

GoStaticSafetySolver::~GoStaticSafetySolver()
{
    if (m_allocRegion)
        delete m_regions;
    // else m_regions belongs to the user, so leave it.
}

bool GoStaticSafetySolver::RegionHealthyForBlock(const GoRegion& r,
                                          const GoBlock& b) const
{
    return b.AllEmptyAreLiberties(r.Points());
}

bool GoStaticSafetySolver::UpToDate() const
{
    return Regions()->UpToDate();
}

void GoStaticSafetySolver::GenBlocksRegions()
{
    if (UpToDate())
        Regions()->ReInitializeBlocksRegions();
    else
    {
        Regions()->GenBlocksRegions();
    }
}


void GoStaticSafetySolver::FindHealthy()
{
    if (Regions()->ComputedHealthy())
        return;
    for (SgBWIterator it; it; ++it)
    {
        SgBlackWhite color(*it);
        for (SgListIteratorOf<GoRegion> it(AllRegions(color)); it; ++it)
        {
            GoRegion* r = *it;
            for (SgListIteratorOf<GoBlock> it2(r->Blocks()); it2; ++it2)
            {
                if (RegionHealthyForBlock(*r, **it2)) // virtual call
                {
                    (*it2)->AddHealthy(r);
                }
            }
        }
    }
    Regions()->SetComputedHealthy();
}

void GoStaticSafetySolver::TestAdjacent(SgListOf<GoRegion>* regions,
                               const SgListOf<GoBlock>& blocks) const
{
    SgListOf<GoRegion> newregions;
    for (SgListIteratorOf<GoRegion> it(*regions); it; ++it)
        if ((*it)->IsSurrounded(blocks))
            newregions.Append(*it);
    *regions = newregions;
}

void GoStaticSafetySolver::TestAlive(SgListOf<GoBlock>* blocks,
                              SgBWSet* safe,
                              SgBlackWhite color)
{
    SgListOf<GoRegion> regions(AllRegions(color));
    SgListOf<GoBlock> toDelete;

    bool changed = true;
    while (changed)
    {
        TestAdjacent(&regions, *blocks);
        toDelete.Clear();
        for (SgListIteratorOf<GoBlock> it(*blocks); it; ++it)
        {
            const SgListOf<GoRegion>& br = (*it)->Healthy();

            bool has2 = br.MinLength(2);
            if (has2)
            {
                int nuRegions = 0;
                for (SgListIteratorOf<GoRegion> it(br); it; ++it)
                {
                    if (regions.Contains(*it))
                    {
                        ++nuRegions;
                        if (nuRegions >= 2)
                            break;
                    }
                }
                has2 = (nuRegions >= 2);
            }
            if (! has2)
                toDelete.Append(*it);
        }

        changed = toDelete.NonEmpty();
        if (changed)
        {
            for (SgListIteratorOf<GoBlock> it(toDelete); it; ++it)
            {
                bool found = blocks->Exclude(*it);
                SG_UNUSED(found);
                SG_ASSERT(found);
            }
        }
    }

    if (blocks->NonEmpty()) // found safe blocks
    {
        SgPointSet blockPoints;
        for (SgListIteratorOf<GoBlock> it(*blocks); it; ++it)
        {
            blockPoints |= (*it)->Stones();
            (*it)->SetToSafe();
        }

        color = blocks->Top()->Color();
        AddToSafe(m_board, blockPoints, color, safe,
                  "TestAlive-Blocks", 0, false);

        for (SgListIteratorOf<GoRegion> it(regions); it; ++it)
            if ((*it)->HealthyForSomeBlock(*blocks))
            {
                (*it)->SetToSafe();
                AddToSafe(m_board, (*it)->Points(), color, safe,
                          "TestAlive-Region", 0, false);
            }
    }
}

void GoStaticSafetySolver::FindClosure(SgListOf<GoBlock>* blocks) const
{
    SgListOf<GoBlock> toTest(*blocks);
    while (toTest.NonEmpty())
    {
        const GoBlock* b = toTest.Pop();
        for (SgListIteratorOf<GoRegion> it(b->Healthy()); it; ++it)
        {   GoRegion* r = *it;
            for (SgListIteratorOf<GoBlock> it(r->Blocks()); it; ++it)
            {   GoBlock* b2 = *it;
                if (! blocks->Contains(b2) && b2->ContainsHealthy(r))
                {
                    blocks->Append(b2);
                    toTest.Append(b2);
                }
            }
        }
    }
}

void GoStaticSafetySolver::FindTestSets(SgListOf<SgListOf<GoBlock> >* sets,
                                 SgBlackWhite color) const
{
    SG_ASSERT(sets->IsEmpty());
    SgListOf<GoBlock> doneSoFar;
    for (SgListIteratorOf<GoBlock> it(AllBlocks(color)); it; ++it)
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

void GoStaticSafetySolver::FindSafePoints(SgBWSet* safe)
{
    GenBlocksRegions(); // find all blocks and regions on whole board
    FindHealthy(); // find healthy regions of blocks

    for (SgBWIterator it; it; ++it)
    {
        SgBlackWhite color(*it);
        SgListOf<SgListOf<GoBlock> > sets;
        // find maximal sets for aliveness testing
        FindTestSets(&sets, color);

        for (SgListIteratorOf<SgListOf<GoBlock> > it(sets); it; ++it)
        {   TestAlive(*it, safe, color);
            // find safe subset within each maximal set
            delete *it;
        }
    }

    Regions()->SetComputedFlagForAll(isSafe);
}
