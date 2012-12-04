/*
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU Library General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 *
 * SLPStoreTest.cpp
 * Test fixture for the SLPStore class
 * Copyright (C) 2012 Simon Newton
 */

#include <cppunit/Asserter.h>
#include <cppunit/SourceLine.h>
#include <cppunit/extensions/HelperMacros.h>
#include <string>

#include "ola/Clock.h"
#include "ola/Logging.h"
#include "ola/testing/TestUtils.h"
#include "tools/slp/SLPStore.h"
#include "tools/slp/ScopeSet.h"
#include "tools/slp/ServiceEntry.h"

using ola::MockClock;
using ola::TimeStamp;
using ola::slp::SLPStore;
using ola::slp::ScopeSet;
using ola::slp::ServiceEntries;
using ola::slp::ServiceEntry;
using ola::slp::URLEntries;
using std::string;


class SLPStoreTest: public CppUnit::TestFixture {
  CPPUNIT_TEST_SUITE(SLPStoreTest);
  CPPUNIT_TEST(testInsertAndLookup);
  CPPUNIT_TEST(testURLEntryLookup);
  CPPUNIT_TEST(testDoubleInsert);
  CPPUNIT_TEST(testRemove);
  CPPUNIT_TEST(testAging);
  CPPUNIT_TEST_SUITE_END();

  public:
    void testInsertAndLookup();
    void testURLEntryLookup();
    void testDoubleInsert();
    void testRemove();
    void testAging();

    void setUp() {
      ola::InitLogging(ola::OLA_LOG_INFO, ola::OLA_LOG_STDERR);
      m_clock.CurrentTime(&now);
      test_scopes = ScopeSet("scope1,scope2");
      disjoint_scopes = ScopeSet("scopes3");
    }

    static const char SCOPE1[];
    static const char SCOPE2[];
    static const char SCOPE3[];
    static const char SERVICE1[];
    static const char SERVICE2[];
    static const char SERVICE1_URL1[];
    static const char SERVICE1_URL2[];
    static const char SERVICE2_URL1[];
    static const char SERVICE2_URL2[];

  private:
    SLPStore m_store;
    MockClock m_clock;
    TimeStamp now;
    ScopeSet test_scopes;
    ScopeSet disjoint_scopes;

    void AdvanceTime(unsigned int seconds) {
      m_clock.AdvanceTime(seconds, 0);
      m_clock.CurrentTime(&now);
    }
};

const char SLPStoreTest::SCOPE1[] = "scope1";
const char SLPStoreTest::SCOPE2[] = "scope2";
const char SLPStoreTest::SCOPE3[] = "scope3";
const char SLPStoreTest::SERVICE1[] = "service:one";
const char SLPStoreTest::SERVICE2[] = "service:two";
const char SLPStoreTest::SERVICE1_URL1[] = "service:one://192.168.1.1";
const char SLPStoreTest::SERVICE1_URL2[] = "service:one://192.168.1.2";
const char SLPStoreTest::SERVICE2_URL1[] = "service:two://192.168.1.1";
const char SLPStoreTest::SERVICE2_URL2[] = "service:two://192.168.1.3";


/**
 * A helper function to check that two ServiceEntries match
 */
void AssertURLEntriesMatch(const CPPUNIT_NS::SourceLine &source_line,
                               const URLEntries &expected,
                               const URLEntries &actual) {
  CPPUNIT_NS::assertEquals(expected.size(), actual.size(), source_line,
                            "ServiceEntries sizes not equal");

  URLEntries::const_iterator iter1 = expected.begin();
  URLEntries::const_iterator iter2 = actual.begin();
  while (iter1 != expected.end()) {
    CPPUNIT_NS::assertEquals(*iter1, *iter2, source_line,
                             "ServiceEntries element not equal");
    CPPUNIT_NS::assertEquals(iter1->lifetime(), iter2->lifetime(), source_line,
                             "ServiceEntries elements lifetime not equal");
    iter1++;
    iter2++;
  }
}

#define OLA_ASSERT_URLENTRIES_EQ(expected, output)  \
  AssertURLEntriesMatch(CPPUNIT_SOURCELINE(), (expected), (output))

CPPUNIT_TEST_SUITE_REGISTRATION(SLPStoreTest);

/*
 * Check that we can insert and lookup entries.
 */
void SLPStoreTest::testInsertAndLookup() {
  OLA_ASSERT_EQ(0u, m_store.ServiceCount());

  // first we should get nothing for either service
  URLEntries urls, expected_urls;
  m_store.Lookup(now, test_scopes, SERVICE1, &urls);
  OLA_ASSERT_TRUE(urls.empty());
  m_store.Lookup(now, test_scopes, SERVICE2, &urls);
  OLA_ASSERT_TRUE(urls.empty());

  // insert a service and confirm it's there
  ServiceEntry service1(test_scopes, SERVICE1_URL1, 10);
  OLA_ASSERT_EQ(SLPStore::OK, m_store.Insert(now, service1));
  m_store.Lookup(now, test_scopes, SERVICE1, &urls);
  expected_urls.push_back(service1.url());
  OLA_ASSERT_URLENTRIES_EQ(expected_urls, urls);
  urls.clear();

  // try the same service in different scopes
  m_store.Lookup(now, disjoint_scopes, SERVICE1, &urls);
  OLA_ASSERT_TRUE(urls.empty());

  // the second service should still be empty in both scopes
  m_store.Lookup(now, test_scopes, SERVICE2, &urls);
  OLA_ASSERT_TRUE(urls.empty());
  m_store.Lookup(now, disjoint_scopes, SERVICE2, &urls);
  OLA_ASSERT_TRUE(urls.empty());

  // insert a second entry for the same service
  ServiceEntry service2(test_scopes, SERVICE1_URL2, 10);
  OLA_ASSERT_EQ(SLPStore::OK, m_store.Insert(now, service2));
  m_store.Lookup(now, test_scopes, SERVICE1, &urls);
  expected_urls.push_back(service2.url());
  OLA_ASSERT_URLENTRIES_EQ(expected_urls, urls);

  // insert an entry for a different service
  ServiceEntry service3(test_scopes, SERVICE2_URL1, 10);
  OLA_ASSERT_EQ(SLPStore::OK, m_store.Insert(now, service3));

  // check that the first service still returns the correct results
  urls.clear();
  m_store.Lookup(now, test_scopes, SERVICE1, &urls);
  OLA_ASSERT_URLENTRIES_EQ(expected_urls, urls);

  // check the second service is there
  urls.clear();
  m_store.Lookup(now, test_scopes, SERVICE2, &urls);
  expected_urls.clear();
  expected_urls.push_back(service3.url());
  OLA_ASSERT_URLENTRIES_EQ(expected_urls, urls);

  // but again, not for the other scopes
  urls.clear();
  m_store.Lookup(now, disjoint_scopes, SERVICE2, &urls);
  OLA_ASSERT_TRUE(urls.empty());
}


/*
 * Check that the URLEntry form of Lookup works.
 */
void SLPStoreTest::testURLEntryLookup() {
  // first we should get nothing for either service
  URLEntries urls, expected_urls;
  m_store.Lookup(now, test_scopes, SERVICE1, &urls);
  OLA_ASSERT_TRUE(urls.empty());
  m_store.Lookup(now, test_scopes, SERVICE2, &urls);
  OLA_ASSERT_TRUE(urls.empty());

  // insert a service and confirm it's there
  ServiceEntry service1(test_scopes, SERVICE1_URL1, 10);
  OLA_ASSERT_EQ(SLPStore::OK, m_store.Insert(now, service1));
  m_store.Lookup(now, test_scopes, SERVICE1, &urls);
  expected_urls.push_back(service1.url());
  OLA_ASSERT_EQ(expected_urls.size(), urls.size());
  OLA_ASSERT_EQ(*(expected_urls.begin()), *(urls.begin()));
}


/*
 * Insert an entry into the Store twice. This checks we take the higher lifetime
 * of two entries as long as the scope list is the same
 */
void SLPStoreTest::testDoubleInsert() {
  URLEntries urls, expected_urls;
  m_store.Lookup(now, test_scopes, SERVICE1, &urls);

  ServiceEntry service(test_scopes, SERVICE1_URL1, 10);
  ServiceEntry service_shorter(test_scopes, SERVICE1_URL1, 5);
  ServiceEntry service_longer(test_scopes, SERVICE1_URL1, 20);
  OLA_ASSERT_EQ(SLPStore::OK, m_store.Insert(now, service));

  m_store.Lookup(now, test_scopes, SERVICE1, &urls);
  expected_urls.push_back(service.url());
  OLA_ASSERT_URLENTRIES_EQ(expected_urls, urls);

  // now insert the shorter one
  urls.clear();
  OLA_ASSERT_EQ(SLPStore::OK, m_store.Insert(now, service_shorter));
  m_store.Lookup(now, test_scopes, SERVICE1, &urls);
  OLA_ASSERT_URLENTRIES_EQ(expected_urls, urls);

  // now insert the longer one
  urls.clear();
  OLA_ASSERT_EQ(SLPStore::OK, m_store.Insert(now, service_longer));
  expected_urls.clear();
  expected_urls.push_back(service_longer.url());
  m_store.Lookup(now, test_scopes, SERVICE1, &urls);
  OLA_ASSERT_URLENTRIES_EQ(expected_urls, urls);

  // Inserting the same url with different scopes should fail
  ServiceEntry different_scopes_service(disjoint_scopes, SERVICE1_URL1, 10);
  OLA_ASSERT_EQ(SLPStore::SCOPE_MISMATCH,
                m_store.Insert(now, different_scopes_service));
}


/*
 * Test the bulk loader.
void SLPStoreTest::testBulkInsert() {
  ServiceEntries entries_to_insert;
  ServiceEntry service(test_scopes, SERVICE1_URL1, 10);
  ServiceEntry service2(test_scopes, SERVICE1_URL2, 10);
  entries_to_insert.push_back(service);
  entries_to_insert.push_back(service2);
  OLA_ASSERT_TRUE(m_store.BulkInsert(now, entries_to_insert));

  URLEntries urls, expected_urls;
  expected_urls.push_back(service.url());
  expected_urls.push_back(service2.url());
  m_store.Lookup(now, test_scopes, SERVICE1, &urls);
  OLA_ASSERT_URLENTRIES_EQ(expected_urls, urls);

  m_store.Reset();
  entries_to_insert.clear();
  expected_urls.clear();
  urls.clear();

  // now try it with Entries that have different urls
  ServiceEntry service3(test_scopes, SERVICE2_URL1, 10);
  entries_to_insert.push_back(service);
  entries_to_insert.push_back(service3);
  OLA_ASSERT_FALSE(m_store.BulkInsert(now, entries_to_insert));
  expected_urls.push_back(service.url());
  m_store.Lookup(now, test_scopes, SERVICE1, &urls);
  OLA_ASSERT_URLENTRIES_EQ(expected_urls, urls);

  m_store.Reset();
  entries_to_insert.clear();
  expected_urls.clear();
  urls.clear();

  // now try it with Entries with different scopes, this should be fine
  ServiceEntry service4(disjoint_scopes, SERVICE1_URL2, 10);
  entries_to_insert.insert(service);
  entries_to_insert.insert(service4);
  OLA_ASSERT_TRUE(m_store.BulkInsert(now, entries_to_insert));

  expected_urls.insert(service);
  m_store.Lookup(now, test_scopes, SERVICE1, &urls);
  OLA_ASSERT_URLENTRIES_EQ(expected_urls, urls);

  expected_urls.clear();
  urls.clear();

  expected_urls.insert(service4);
  m_store.Lookup(now, disjoint_scopes, SERVICE1, &urls);
  OLA_ASSERT_URLENTRIES_EQ(expected_urls, urls);
}
 */


/**
 * Test Remove()
 */
void SLPStoreTest::testRemove() {
  URLEntries urls, expected_urls;
  ServiceEntry service1(test_scopes, SERVICE1_URL1, 10);
  OLA_ASSERT_EQ(SLPStore::OK, m_store.Insert(now, service1));

  // verify it's there
  m_store.Lookup(now, test_scopes, SERVICE1, &urls);
  expected_urls.push_back(service1.url());
  OLA_ASSERT_URLENTRIES_EQ(expected_urls, urls);
  expected_urls.clear();
  urls.clear();

  // now try to remove it with a different set of scopes
  ServiceEntry different_scopes_service(disjoint_scopes, SERVICE1_URL1, 10);
  OLA_ASSERT_EQ(SLPStore::SCOPE_MISMATCH,
                m_store.Remove(different_scopes_service));

  // verify it's still there
  m_store.Lookup(now, test_scopes, SERVICE1, &urls);
  expected_urls.push_back(service1.url());
  OLA_ASSERT_URLENTRIES_EQ(expected_urls, urls);
  expected_urls.clear();
  urls.clear();

  // now actually remove it
  OLA_ASSERT_EQ(SLPStore::OK, m_store.Remove(service1));

  // confirm it's no longer there
  m_store.Lookup(now, test_scopes, SERVICE1, &urls);
  OLA_ASSERT_TRUE(urls.empty());

  // the number of urls should be zero, which indicates we've cleaned up
  // the service map correctly.
  OLA_ASSERT_EQ(0u, m_store.ServiceCount());
}


/*
 * Test aging.
 */
void SLPStoreTest::testAging() {
  ServiceEntry service(test_scopes, SERVICE1_URL1, 10);
  ServiceEntry short_service(test_scopes, SERVICE1_URL1, 5);
  ServiceEntry service2(test_scopes, SERVICE2_URL1, 10);
  ServiceEntry short_service2(test_scopes, SERVICE2_URL1, 5);
  OLA_ASSERT_EQ(SLPStore::OK, m_store.Insert(now, service));

  AdvanceTime(10);

  URLEntries urls, expected_urls;
  m_store.Lookup(now, test_scopes, SERVICE1, &urls);
  OLA_ASSERT_TRUE(urls.empty());

  // insert it again
  OLA_ASSERT_EQ(SLPStore::OK, m_store.Insert(now, service));
  AdvanceTime(5);
  // insert an entry for the second service
  OLA_ASSERT_EQ(SLPStore::OK, m_store.Insert(now, service2));

  m_store.Lookup(now, test_scopes, SERVICE1, &urls);
  expected_urls.push_back(short_service.url());
  OLA_ASSERT_URLENTRIES_EQ(expected_urls, urls);

  urls.clear();
  expected_urls.clear();
  m_store.Lookup(now, test_scopes, SERVICE2, &urls);
  expected_urls.push_back(service2.url());
  OLA_ASSERT_URLENTRIES_EQ(expected_urls, urls);

  AdvanceTime(5);
  expected_urls.clear();
  urls.clear();
  m_store.Lookup(now, test_scopes, SERVICE1, &urls);
  OLA_ASSERT_TRUE(urls.empty());

  m_store.Lookup(now, test_scopes, SERVICE2, &urls);
  expected_urls.push_back(short_service2.url());
  OLA_ASSERT_URLENTRIES_EQ(expected_urls, urls);

  AdvanceTime(5);
  expected_urls.clear();
  urls.clear();
  m_store.Lookup(now, test_scopes, SERVICE1, &urls);
  OLA_ASSERT_TRUE(urls.empty());
  m_store.Lookup(now, test_scopes, SERVICE2, &urls);
  OLA_ASSERT_TRUE(urls.empty());
}
