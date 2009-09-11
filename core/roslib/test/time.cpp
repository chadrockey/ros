/*
 * Copyright (c) 2008, Willow Garage, Inc.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in the
 *       documentation and/or other materials provided with the distribution.
 *     * Neither the name of Willow Garage, Inc. nor the names of its
 *       contributors may be used to endorse or promote products derived from
 *       this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include <vector>

#include <gtest/gtest.h>
#include <ros/time.h>
#include <sys/time.h>

using namespace ros;

/// \todo All the tests in here that use randomized values are not unit tests, replace them

double epsilon = 1e-9;

void seed_rand()
{
  //Seed random number generator with current microseond count
  timeval temp_time_struct;
  gettimeofday(&temp_time_struct,NULL);
  srand(temp_time_struct.tv_usec);
};

void generate_rand_times(uint32_t range, uint64_t runs, std::vector<ros::Time>& values1, std::vector<ros::Time>& values2)
{
  seed_rand();
  values1.clear();
  values2.clear();
  values1.reserve(runs);
  values2.reserve(runs);
  for ( uint32_t i = 0; i < runs ; i++ )
  {
    values1.push_back(ros::Time( (rand() * range / RAND_MAX), (rand() * 1000000000ULL/RAND_MAX)));
    values2.push_back(ros::Time( (rand() * range / RAND_MAX), (rand() * 1000000000ULL/RAND_MAX)));
  }
}

void generate_rand_durations(uint32_t range, uint64_t runs, std::vector<ros::Duration>& values1, std::vector<ros::Duration>& values2)
{
  seed_rand();
  values1.clear();
  values2.clear();
  values1.reserve(runs);
  values2.reserve(runs);
  for ( uint32_t i = 0; i < runs ; i++ )
  {
    values1.push_back(ros::Duration( (rand() * range / RAND_MAX), (rand() * 1000000000ULL/RAND_MAX)));
    values2.push_back(ros::Duration( (rand() * range / RAND_MAX), (rand() * 1000000000ULL/RAND_MAX)));
  }
}

TEST(Time, Comparitors)
{
  std::vector<ros::Time> v1;
  std::vector<ros::Time> v2;
  generate_rand_times(100, 1000, v1,v2);

  for (uint32_t i = 0; i < v1.size(); i++)
  {
    if (v1[i].sec * 1000000000ULL + v1[i].nsec < v2[i].sec * 1000000000ULL + v2[i].nsec)
    {
      EXPECT_LT(v1[i], v2[i]);
      //      printf("%f %d ", v1[i].toSec(), v1[i].sec * 1000000000ULL + v1[i].nsec);
      //printf("vs %f %d\n", v2[i].toSec(), v2[i].sec * 1000000000ULL + v2[i].nsec);
      EXPECT_LE(v1[i], v2[i]);
      EXPECT_NE(v1[i], v2[i]);
    }
    else if (v1[i].sec * 1000000000ULL + v1[i].nsec > v2[i].sec * 1000000000ULL + v2[i].nsec)
    {
      EXPECT_GT(v1[i], v2[i]);
      EXPECT_GE(v1[i], v2[i]);
      EXPECT_NE(v1[i], v2[i]);
    }
    else
    {
      EXPECT_EQ(v1[i], v2[i]);
      EXPECT_LE(v1[i], v2[i]);
      EXPECT_GE(v1[i], v2[i]);
    }

  }

}

TEST(Time, ToFromDouble)
{
  std::vector<ros::Time> v1;
  std::vector<ros::Time> v2;
  generate_rand_times(100, 1000, v1,v2);

  for (uint32_t i = 0; i < v1.size(); i++)
  {
    EXPECT_EQ(v1[i].toSec(), v1[i].fromSec(v1[i].toSec()).toSec());

  }

}

TEST(Time, OperatorPlus)
{
  Time t(100, 0);
  Duration d(100, 0);
  Time r = t + d;
  EXPECT_EQ(r.sec, 200UL);
  EXPECT_EQ(r.nsec, 0UL);

  t = Time(0, 100000UL);
  d = Duration(0, 100UL);
  r = t + d;
  EXPECT_EQ(r.sec, 0UL);
  EXPECT_EQ(r.nsec, 100100UL);

  t = Time(0, 0);
  d = Duration(10, 2000003000UL);
  r = t + d;
  EXPECT_EQ(r.sec, 12UL);
  EXPECT_EQ(r.nsec, 3000UL);
}

TEST(Time, OperatorMinus)
{
  Time t(100, 0);
  Duration d(100, 0);
  Time r = t - d;
  EXPECT_EQ(r.sec, 0UL);
  EXPECT_EQ(r.nsec, 0UL);

  t = Time(0, 100000UL);
  d = Duration(0, 100UL);
  r = t - d;
  EXPECT_EQ(r.sec, 0UL);
  EXPECT_EQ(r.nsec, 99900UL);

  t = Time(30, 0);
  d = Duration(10, 2000003000UL);
  r = t - d;
  EXPECT_EQ(r.sec, 17UL);
  EXPECT_EQ(r.nsec, 999997000ULL);
}

TEST(Time, OperatorPlusEquals)
{
  Time t(100, 0);
  Duration d(100, 0);
  t += d;
  EXPECT_EQ(t.sec, 200UL);
  EXPECT_EQ(t.nsec, 0UL);

  t = Time(0, 100000UL);
  d = Duration(0, 100UL);
  t += d;
  EXPECT_EQ(t.sec, 0UL);
  EXPECT_EQ(t.nsec, 100100UL);

  t = Time(0, 0);
  d = Duration(10, 2000003000UL);
  t += d;
  EXPECT_EQ(t.sec, 12UL);
  EXPECT_EQ(t.nsec, 3000UL);
}

TEST(Time, OperatorMinusEquals)
{
  Time t(100, 0);
  Duration d(100, 0);
  t -= d;
  EXPECT_EQ(t.sec, 0UL);
  EXPECT_EQ(t.nsec, 0UL);

  t = Time(0, 100000UL);
  d = Duration(0, 100UL);
  t -= d;
  EXPECT_EQ(t.sec, 0UL);
  EXPECT_EQ(t.nsec, 99900UL);

  t = Time(30, 0);
  d = Duration(10, 2000003000UL);
  t -= d;
  EXPECT_EQ(t.sec, 17UL);
  EXPECT_EQ(t.nsec, 999997000ULL);
}

TEST(Time, SecNSecConstructor)
{
  Time t(100, 2000003000UL);
  EXPECT_EQ(t.sec, 102UL);
  EXPECT_EQ(t.nsec, 3000UL);
}

/************************************* Duration Tests *****************/

TEST(Duration, Comparitors)
{
  std::vector<ros::Duration> v1;
  std::vector<ros::Duration> v2;
  generate_rand_durations(100, 1000, v1,v2);

  for (uint32_t i = 0; i < v1.size(); i++)
  {
    if (v1[i].sec * 1000000000ULL + v1[i].nsec < v2[i].sec * 1000000000ULL + v2[i].nsec)
    {
      EXPECT_LT(v1[i], v2[i]);
      //      printf("%f %d ", v1[i].toSec(), v1[i].sec * 1000000000ULL + v1[i].nsec);
      //printf("vs %f %d\n", v2[i].toSec(), v2[i].sec * 1000000000ULL + v2[i].nsec);
      EXPECT_LE(v1[i], v2[i]);
      EXPECT_NE(v1[i], v2[i]);
    }
    else if (v1[i].sec * 1000000000ULL + v1[i].nsec > v2[i].sec * 1000000000ULL + v2[i].nsec)
    {
      EXPECT_GT(v1[i], v2[i]);
      EXPECT_GE(v1[i], v2[i]);
      EXPECT_NE(v1[i], v2[i]);
    }
    else
    {
      EXPECT_EQ(v1[i], v2[i]);
      EXPECT_LE(v1[i], v2[i]);
      EXPECT_GE(v1[i], v2[i]);
    }

  }

}

TEST(Duration, ToFromSec)
{
  std::vector<ros::Duration> v1;
  std::vector<ros::Duration> v2;
  generate_rand_durations(100, 1000, v1,v2);

  for (uint32_t i = 0; i < v1.size(); i++)
  {
    EXPECT_EQ(v1[i].toSec(), v1[i].fromSec(v1[i].toSec()).toSec());

  }

}


TEST(Duration, OperatorPlus)
{
  std::vector<ros::Duration> v1;
  std::vector<ros::Duration> v2;
  generate_rand_durations(100, 1000, v1,v2);

  for (uint32_t i = 0; i < v1.size(); i++)
  {
    EXPECT_NEAR(v1[i].toSec() + v2[i].toSec(), (v1[i] + v2[i]).toSec(), epsilon);
    ros::Duration temp = v1[i];
    EXPECT_NEAR(v1[i].toSec() + v2[i].toSec(), (temp += v2[i]).toSec(), epsilon);

  }

}

TEST(Duration, OperatorMinus)
{
  std::vector<ros::Duration> v1;
  std::vector<ros::Duration> v2;
  generate_rand_durations(100, 1000, v1,v2);

  for (uint32_t i = 0; i < v1.size(); i++)
  {
    EXPECT_NEAR(v1[i].toSec() - v2[i].toSec(), (v1[i] - v2[i]).toSec(), epsilon);
    ros::Duration temp = v1[i];
    EXPECT_NEAR(v1[i].toSec() - v2[i].toSec(), (temp -= v2[i]).toSec(), epsilon);

    EXPECT_NEAR(- v2[i].toSec(), (-v2[i]).toSec(), epsilon);

  }

}

TEST(Duration, OperatorTimes)
{
  std::vector<ros::Duration> v1;
  std::vector<ros::Duration> v2;
  generate_rand_durations(100, 1000, v1,v2);

  for (uint32_t i = 0; i < v1.size(); i++)
  {
    EXPECT_NEAR(v1[i].toSec() * v2[i].toSec(), (v1[i] * v2[i].toSec()).toSec(), epsilon);
    ros::Duration temp = v1[i];
    EXPECT_NEAR(v1[i].toSec() * v2[i].toSec(), (temp *= v2[i].toSec()).toSec(), epsilon);

  }

}

TEST(Duration, OperatorPlusEquals)
{
  Duration t(100, 0);
  Duration d(100, 0);
  t += d;
  EXPECT_EQ(t.sec, 200L);
  EXPECT_EQ(t.nsec, 0L);

  t = Duration(0, 100000L);
  d = Duration(0, 100L);
  t += d;
  EXPECT_EQ(t.sec, 0L);
  EXPECT_EQ(t.nsec, 100100L);

  t = Duration(0, 0);
  d = Duration(10, 2000003000L);
  t += d;
  EXPECT_EQ(t.sec, 12L);
  EXPECT_EQ(t.nsec, 3000L);
}

TEST(Duration, OperatorMinusEquals)
{
  Duration t(100, 0);
  Duration d(100, 0);
  t -= d;
  EXPECT_EQ(t.sec, 0L);
  EXPECT_EQ(t.nsec, 0L);

  t = Duration(0, 100000L);
  d = Duration(0, 100L);
  t -= d;
  EXPECT_EQ(t.sec, 0L);
  EXPECT_EQ(t.nsec, 99900L);

  t = Duration(30, 0);
  d = Duration(10, 2000003000L);
  t -= d;
  EXPECT_EQ(t.sec, 17L);
  EXPECT_EQ(t.nsec, 999997000L);
}

///////////////////////////////////////////////////////////////////////////////////
// WallTime/WallDuration
///////////////////////////////////////////////////////////////////////////////////


int main(int argc, char **argv){
  testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
