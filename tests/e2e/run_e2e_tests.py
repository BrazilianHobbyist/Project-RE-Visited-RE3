#!/usr/bin/env python3
"""
Master E2E Test Runner for RE3Recomp HD Texture & 3D Model Pack Engine.
Executes all test tiers (Tiers 1-5) and generates human-readable and CI reports.
"""

import argparse
import os
import sys
import unittest

# Ensure project root is in sys.path
PROJECT_ROOT = os.path.abspath(os.path.join(os.path.dirname(__file__), "..", ".."))
if PROJECT_ROOT not in sys.path:
    sys.path.insert(0, PROJECT_ROOT)

from tests.e2e.harness.test_framework import E2ETestRunner
from tests.e2e import test_tier1_features
from tests.e2e import test_tier2_boundaries
from tests.e2e import test_tier3_interactions
from tests.e2e import test_tier4_scenarios
from tests.e2e import test_tier5_adversarial


def build_full_suite(tier_filter=None, keyword=None) -> unittest.TestSuite:
    suite = unittest.TestSuite()
    loader = unittest.TestLoader()

    modules = []
    if tier_filter is None:
        modules = [
            test_tier1_features,
            test_tier2_boundaries,
            test_tier3_interactions,
            test_tier4_scenarios,
            test_tier5_adversarial,
        ]
    elif tier_filter == 1:
        modules = [test_tier1_features]
    elif tier_filter == 2:
        modules = [test_tier2_boundaries]
    elif tier_filter == 3:
        modules = [test_tier3_interactions]
    elif tier_filter == 4:
        modules = [test_tier4_scenarios]
    elif tier_filter == 5:
        modules = [test_tier5_adversarial]
    else:
        print(f"Error: Unknown tier {tier_filter}. Expected 1..5.")
        sys.exit(1)

    for mod in modules:
        mod_suite = loader.loadTestsFromModule(mod)
        if keyword:
            filtered = unittest.TestSuite()
            for t in _flatten_suite(mod_suite):
                test_name = f"{t.__class__.__name__}.{t._testMethodName}"
                if keyword.lower() in test_name.lower():
                    filtered.addTest(t)
            suite.addTest(filtered)
        else:
            suite.addTest(mod_suite)

    return suite


def _flatten_suite(suite):
    tests = []
    for item in suite:
        if isinstance(item, unittest.TestSuite):
            tests.extend(_flatten_suite(item))
        elif isinstance(item, unittest.TestCase):
            tests.append(item)
    return tests


def main():
    parser = argparse.ArgumentParser(description="RE3Recomp E2E Test Suite Runner")
    parser.add_argument("--tier", type=int, choices=[1, 2, 3, 4, 5], help="Run tests for specific tier only (1-5)")
    parser.add_argument("-v", "--verbose", action="store_true", help="Verbose test execution output")
    parser.add_argument("-k", "--keyword", type=str, help="Filter tests matching keyword / substring")
    parser.add_argument("--tap", action="store_true", help="Output in Test Anything Protocol format")
    parser.add_argument("--junit", type=str, help="Path to write JUnit XML report")

    args = parser.parse_args()

    suite = build_full_suite(tier_filter=args.tier, keyword=args.keyword)
    runner = E2ETestRunner(verbose=args.verbose, tap=args.tap, junit_path=args.junit)
    success = runner.run_suite(suite, tier_filter=args.tier)

    sys.exit(0 if success else 1)


if __name__ == "__main__":
    main()
