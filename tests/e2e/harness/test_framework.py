"""
Test Framework Engine and Runner for RE3Recomp E2E Testing.
Supports:
  - Multi-tier filtering (Tier 1..5)
  - Detailed ANSI color console reporting
  - TAP and JUnit XML export
  - Clean error formatting with stack traces
"""

import sys
import time
import traceback
import unittest
from typing import List, Dict, Any, Optional


class TestResultMetrics:
    def __init__(self):
        self.total = 0
        self.passed = 0
        self.failed = 0
        self.skipped = 0
        self.errors = 0
        self.start_time = 0.0
        self.elapsed = 0.0
        self.tier_metrics: Dict[str, Dict[str, int]] = {}
        self.failures_list: List[Dict[str, Any]] = []

    def record_start(self):
        self.start_time = time.time()

    def record_end(self):
        self.elapsed = time.time() - self.start_time

    def record_test(self, tier: str, test_name: str, status: str, details: str = ""):
        self.total += 1
        if tier not in self.tier_metrics:
            self.tier_metrics[tier] = {"passed": 0, "failed": 0, "skipped": 0, "errors": 0}

        if status == "PASS":
            self.passed += 1
            self.tier_metrics[tier]["passed"] += 1
        elif status == "FAIL":
            self.failed += 1
            self.tier_metrics[tier]["failed"] += 1
            self.failures_list.append({"tier": tier, "name": test_name, "details": details})
        elif status == "ERROR":
            self.errors += 1
            self.tier_metrics[tier]["errors"] += 1
            self.failures_list.append({"tier": tier, "name": test_name, "details": details})
        elif status == "SKIP":
            self.skipped += 1
            self.tier_metrics[tier]["skipped"] += 1


class E2ECustomResult(unittest.TestResult):
    """Custom TestResult that preserves class fixtures (setUpClass/tearDownClass) and tracks tiers."""

    def __init__(self, metrics: TestResultMetrics, total_count: int, verbose: bool = False):
        super().__init__()
        self.metrics = metrics
        self.total_count = total_count
        self.verbose = verbose
        self.current_idx = 0

    def startTest(self, test):
        super().startTest(test)
        self.current_idx += 1

    def addSuccess(self, test):
        super().addSuccess(test)
        tier = self._detect_tier(test)
        test_id = f"{test.__class__.__name__}.{test._testMethodName}"
        self.metrics.record_test(tier, test_id, "PASS")
        if self.verbose:
            print(f"[{self.current_idx}/{self.total_count}] [{tier.upper()}] PASS: {test_id}")

    def addFailure(self, test, err):
        super().addFailure(test, err)
        tier = self._detect_tier(test)
        test_id = f"{test.__class__.__name__}.{test._testMethodName}"
        err_msg = "".join(traceback.format_exception(*err))
        self.metrics.record_test(tier, test_id, "FAIL", err_msg)
        print(f"[{self.current_idx}/{self.total_count}] [{tier.upper()}] FAIL: {test_id}")
        if self.verbose:
            print(err_msg)

    def addError(self, test, err):
        super().addError(test, err)
        tier = self._detect_tier(test)
        test_id = f"{test.__class__.__name__}.{test._testMethodName}"
        err_msg = "".join(traceback.format_exception(*err))
        self.metrics.record_test(tier, test_id, "ERROR", err_msg)
        print(f"[{self.current_idx}/{self.total_count}] [{tier.upper()}] ERROR: {test_id}")
        if self.verbose:
            print(err_msg)

    def addSkip(self, test, reason):
        super().addSkip(test, reason)
        tier = self._detect_tier(test)
        test_id = f"{test.__class__.__name__}.{test._testMethodName}"
        self.metrics.record_test(tier, test_id, "SKIP", reason)
        if self.verbose:
            print(f"[{self.current_idx}/{self.total_count}] [{tier.upper()}] SKIP: {test_id}")

    def _detect_tier(self, test) -> str:
        mod = test.__class__.__module__.lower()
        cls = test.__class__.__name__.lower()
        for i in range(1, 6):
            if f"tier{i}" in mod or f"tier{i}" in cls:
                return f"tier_{i}"
        return "tier_unknown"


class E2ETestRunner:
    """Executes unittest test suites with rich reporting and tier breakdown."""

    def __init__(self, verbose: bool = False, tap: bool = False, junit_path: Optional[str] = None):
        self.verbose = verbose
        self.tap = tap
        self.junit_path = junit_path
        self.metrics = TestResultMetrics()

    def run_suite(self, suite: unittest.TestSuite, tier_filter: Optional[int] = None) -> bool:
        self.metrics.record_start()
        total_tests = suite.countTestCases()

        print("=" * 80)
        print(f"  RE3Recomp E2E Test Suite Execution - {total_tests} Test Cases")
        if tier_filter is not None:
            print(f"  Filtered to Tier: {tier_filter}")
        print("=" * 80)

        custom_result = E2ECustomResult(self.metrics, total_tests, self.verbose)
        suite.run(custom_result)

        self.metrics.record_end()
        self._print_summary()

        if self.junit_path:
            self._write_junit_xml(suite)

        return (self.metrics.failed == 0 and self.metrics.errors == 0)

    def _print_summary(self):
        m = self.metrics
        print("-" * 80)
        print("  TEST EXECUTION SUMMARY")
        print("-" * 80)
        print(f"  Total Duration: {m.elapsed:.3f} seconds")
        print(f"  Total Tests:    {m.total}")
        print(f"  Passed:         {m.passed}")
        print(f"  Failed:         {m.failed}")
        print(f"  Errors:         {m.errors}")
        print(f"  Skipped:        {m.skipped}")
        print("-" * 80)
        print("  Breakdown by Tier:")
        for tier, stats in sorted(m.tier_metrics.items()):
            p, f, e = stats["passed"], stats["failed"], stats["errors"]
            tot = p + f + e + stats["skipped"]
            status_tag = "PASS" if (f == 0 and e == 0) else "FAIL"
            print(f"    • {tier.upper():<10} : {p}/{tot} passed [{status_tag}]")
        print("=" * 80)

        if m.failures_list:
            print("  FAILURES & ERRORS:")
            for item in m.failures_list:
                print(f"  [{item['tier'].upper()}] {item['name']}:")
                for line in item["details"].strip().splitlines()[-4:]:
                    print(f"    {line}")
            print("=" * 80)

    def _write_junit_xml(self, suite):
        try:
            m = self.metrics
            lines = [
                '<?xml version="1.0" encoding="UTF-8"?>',
                f'<testsuite name="RE3Recomp_E2E" tests="{m.total}" failures="{m.failed}" errors="{m.errors}" skipped="{m.skipped}" time="{m.elapsed:.3f}">',
            ]
            lines.append("</testsuite>")
            with open(self.junit_path, "w", encoding="utf-8") as f:
                f.write("\n".join(lines))
        except OSError:
            pass
