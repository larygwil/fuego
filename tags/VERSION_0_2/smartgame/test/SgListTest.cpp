//----------------------------------------------------------------------------
/** @file SgListTest.cpp
    Unit tests for SgList.
*/
//----------------------------------------------------------------------------

#include "SgSystem.h"

#include <boost/test/auto_unit_test.hpp>
#include "SgList.h"

//----------------------------------------------------------------------------

namespace {

BOOST_AUTO_TEST_CASE(SgListTestAppend)
{
    SgList<int> a;
    BOOST_CHECK(a.IsEmpty());
    BOOST_CHECK_EQUAL(a.Length(), 0);
    a.Append(123);
    BOOST_CHECK_EQUAL(a.Length(), 1);
    BOOST_CHECK_EQUAL(a.Top(), 123);
    BOOST_CHECK_EQUAL(a.Tail(), 123);
    a.Append(456);
    BOOST_CHECK_EQUAL(a.Length(), 2);
    BOOST_CHECK_EQUAL(a.Top(), 123);
    BOOST_CHECK_EQUAL(a.Tail(), 456);
    int x = a.Pop();
    BOOST_CHECK_EQUAL(x, 123);
    BOOST_CHECK_EQUAL(a.Top(), 456);
    a.Append(x);
    BOOST_CHECK_EQUAL(a[1], 456);
    BOOST_CHECK_EQUAL(a[2], 123);
}

BOOST_AUTO_TEST_CASE(SgListTestAssign)
{
    SgList<int> a;
    a.Append(123);
    a.Append(444);
    a.Append(789);
    SgList<int> b;
    b = a;
    BOOST_CHECK_EQUAL(b[1], 123);
    BOOST_CHECK_EQUAL(b[2], 444);
    BOOST_CHECK_EQUAL(b[3], 789);
    BOOST_CHECK(a == b);
}

BOOST_AUTO_TEST_CASE(SgListTestAssignElement)
{
    SgList<int> a;
    a.Append(0);
    a.Append(789);
    a[2] = 444;
    BOOST_CHECK_EQUAL(a[2], 444);
}

BOOST_AUTO_TEST_CASE(SgListTestClear)
{
    SgList<int> a;
    a.Append(0);
    a.Append(789);
    a.Append(123);
    a.Clear();
    SG_ASSERT(a.IsEmpty());
}

BOOST_AUTO_TEST_CASE(SgListTestCollectGarbage)
{
    {
        SgList<int> a;
        for (int i = 0; i < 1000; ++i)
            a.Append(i);
    }
    SgList<int> b;
    for (int i = 0; i < 6; ++i)
        b.Append(i);
    int numAlloc, numUsed;
    SgList<int>::CollectGarbage();
    SgList<int>::GetStatistics(&numAlloc, &numUsed);
    BOOST_CHECK_EQUAL(numAlloc, SGLIST_NUM_LIST_REC);
    BOOST_CHECK_EQUAL(numUsed, 6);
    while (b.NonEmpty())
        b.DeleteAt(b.Length());
    BOOST_CHECK(b.IsEmpty());
    SgList<int>::CollectGarbage();
    SgList<int>::GetStatistics(&numAlloc, &numUsed);
    BOOST_CHECK_EQUAL(numAlloc, 0);
    BOOST_CHECK_EQUAL(numUsed, 0);
}

BOOST_AUTO_TEST_CASE(SgListTestConcat)
{
    SgList<int> a;
    a.Append(1);
    a.Append(2);
    a.Append(3);
    SgList<int> b;
    b.Append(3);
    b.Append(2);
    b.Append(1);
    a.Concat(&b);
    BOOST_CHECK(b.IsEmpty());
    BOOST_CHECK(a.IsLength(6));
}

BOOST_AUTO_TEST_CASE(SgListTestExclude)
{
    SgList<int> a;
    a.Append(789);
    a.Append(666);
    a.Append(123);
    BOOST_CHECK(! a.Exclude(555));
    BOOST_CHECK(a.Exclude(666));
    BOOST_CHECK(! a.Contains(666));
}

BOOST_AUTO_TEST_CASE(SgListTestInsert)
{
    SgList<int> a;
    a.Insert(789);
    BOOST_CHECK_EQUAL(a.Length(), 1);
    BOOST_CHECK_EQUAL(a[1], 789);
    BOOST_CHECK(a.Insert(666));
    BOOST_CHECK_EQUAL(a[1], 666);
    BOOST_CHECK(! a.Insert(666));
}

BOOST_AUTO_TEST_CASE(SgListTestIterator)
{
    SgList<int> a;                 
    for (int i = 0; i < 10; ++i)
        a.Append(i);
    SgListIterator<int> iter(a);
    int count = 0;
    while (iter)
    {
        BOOST_CHECK_EQUAL(*iter, count);
        ++count;
        ++iter;
    }
    BOOST_CHECK_EQUAL(count, 10);    
}

BOOST_AUTO_TEST_CASE(SgListTestLargeList)
{
    SgList<int> a;                 
    for (int i = 1; i <= 1000; ++i)
        a.Append(i);
    BOOST_CHECK(a.MinLength(1000));
    BOOST_CHECK(a.Contains(233));
    BOOST_CHECK(a.Contains(234));
    for (int i = 1; i <= 1000; i += 2)
        a.Exclude(i);
    BOOST_CHECK(a.MaxLength(1000));
    BOOST_CHECK(! a.Contains(233));
    BOOST_CHECK(a.Contains(234));
    int y = a.Index(2);
    BOOST_CHECK_EQUAL(y, 1);
    y = a.Index(456);
    BOOST_CHECK_EQUAL(y, 228);
    BOOST_CHECK(a.Contains(456));
    a.DeleteAt(y);
    BOOST_CHECK(! a.Contains(456));
    BOOST_CHECK(a.Contains(2));
    a.DeleteAt(1);
    BOOST_CHECK(! a.Contains(2));
    BOOST_CHECK( a.Contains(1000));
    a.DeleteAt(a.Length());
    BOOST_CHECK(! a.Contains(1000));
}

BOOST_AUTO_TEST_CASE(SgListTestPairIterator)
{
    SgList<int> a;                 
    a.Append(1);
    a.Append(2);
    a.Append(3);
    SgListPairIterator<int> iter(a);
    int e1, e2;
    BOOST_CHECK(iter.NextPair(e1, e2));
    BOOST_CHECK_EQUAL(e1, 1);
    BOOST_CHECK_EQUAL(e2, 2);
    BOOST_CHECK(iter.NextPair(e1, e2));
    BOOST_CHECK_EQUAL(e1, 1);
    BOOST_CHECK_EQUAL(e2, 3);
    BOOST_CHECK(iter.NextPair(e1, e2));
    BOOST_CHECK_EQUAL(e1, 2);
    BOOST_CHECK_EQUAL(e2, 3);
    BOOST_CHECK(! iter.NextPair(e1, e2));
    BOOST_CHECK_EQUAL(e1, 2);
    BOOST_CHECK_EQUAL(e2, 3);
}

BOOST_AUTO_TEST_CASE(SgListTestPush)
{
    SgList<int> a;
    a.Append(0);
    a.Push(1);
    BOOST_CHECK_EQUAL(a[1], 1);
    BOOST_CHECK_EQUAL(a.Length(), 2);
}

BOOST_AUTO_TEST_CASE(SgListTestSetTo)
{
    SgList<int> a;
    a.SetTo(3);
    BOOST_CHECK_EQUAL(a[1], 3);
    BOOST_CHECK_EQUAL(a.Length(), 1);
}

BOOST_AUTO_TEST_CASE(SgListTestSort)
{
    SgList<int> a;
    a.Append(0);
    a.Append(789);
    a.Append(123);
    a.Append(-15);
    a.Sort();
    BOOST_CHECK_EQUAL(a[1], -15);
    BOOST_CHECK_EQUAL(a[2], 0);
    BOOST_CHECK_EQUAL(a[3], 123);
    BOOST_CHECK_EQUAL(a[4], 789);
}

} // namespace

//----------------------------------------------------------------------------
