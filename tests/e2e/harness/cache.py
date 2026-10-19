"""
Asynchronous Background Pre-fetcher & LRU Cache Simulator for RE3Recomp.
Implements the multi-threaded eager room pre-fetcher, memory budget,
and hit/miss/eviction mechanics described in Requirement R1.
"""

from collections import OrderedDict
import threading
import time
from typing import Optional, Dict, Any, List, Tuple
from .res_resolver import ReshdpResolver
from .png_util import validate_png_bytes


class BackgroundTextureEntry:
    def __init__(self, key: str, file_path: str, data: bytes, width: int, height: int):
        self.key = key
        self.file_path = file_path
        self.data = data
        self.width = width
        self.height = height
        self.size_bytes = len(data)
        self.last_accessed = time.time()
        self.gl_tex_id = id(self) & 0xFFFFFFFF


class AsyncBackgroundCache:
    """Thread-safe LRU cache and eager pre-fetcher for HD backgrounds."""

    def __init__(self, resolver: ReshdpResolver, max_memory_mb: int = 256, max_entries: int = 32):
        self.resolver = resolver
        self.max_bytes = max_memory_mb * 1024 * 1024
        self.max_entries = max_entries
        self.current_bytes = 0
        self.cache: OrderedDict[str, BackgroundTextureEntry] = OrderedDict()
        self.lock = threading.Lock()

        # Metrics
        self.hits = 0
        self.misses = 0
        self.evictions = 0
        self.prefetch_count = 0

        # Background worker state
        self._work_queue: List[Tuple[int, int, int]] = []
        self._worker_thread: Optional[threading.Thread] = None
        self._running = False

    def start_worker(self):
        """Start async worker thread."""
        with self.lock:
            if self._running:
                return
            self._running = True
            self._worker_thread = threading.Thread(target=self._worker_loop, daemon=True)
            self._worker_thread.start()

    def stop_worker(self):
        """Stop async worker thread."""
        with self.lock:
            self._running = False
        if self._worker_thread and self._worker_thread.is_alive():
            self._worker_thread.join(timeout=1.0)

    def _worker_loop(self):
        while True:
            item = None
            with self.lock:
                if not self._running:
                    break
                if self._work_queue:
                    item = self._work_queue.pop(0)

            if item:
                stage, room, cut = item
                self._load_sync(stage, room, cut)
            else:
                time.sleep(0.01)

    def on_room_change(self, stage: int, room: int):
        """Eagerly queue pre-fetching of all cuts for this room."""
        cuts = self.resolver.list_room_cuts(stage, room)
        with self.lock:
            for cut_val, _ in cuts:
                key = f"R{stage:01X}{room:02X}_{cut_val:02X}"
                if key not in self.cache and (stage, room, cut_val) not in self._work_queue:
                    self._work_queue.append((stage, room, cut_val))
                    self.prefetch_count += 1

    def _load_sync(self, stage: int, room: int, cut: int) -> Optional[BackgroundTextureEntry]:
        key = f"R{stage:01X}{room:02X}_{cut:02X}"
        file_path = self.resolver.resolve(stage, room, cut)
        if not file_path:
            return None

        try:
            with open(file_path, "rb") as f:
                data = f.read()
            valid, _, meta = validate_png_bytes(data)
            if not valid or not meta:
                return None
            w, h = meta["width"], meta["height"]
        except Exception:
            return None

        entry = BackgroundTextureEntry(key, file_path, data, w, h)
        with self.lock:
            self._insert_entry(key, entry)
        return entry

    def _insert_entry(self, key: str, entry: BackgroundTextureEntry):
        if key in self.cache:
            old = self.cache.pop(key)
            self.current_bytes -= old.size_bytes

        # Evict LRU if budget exceeded
        while self.cache and (
            (self.current_bytes + entry.size_bytes > self.max_bytes)
            or (len(self.cache) >= self.max_entries)
        ):
            _, evicted = self.cache.popitem(last=False)
            self.current_bytes -= evicted.size_bytes
            self.evictions += 1

        self.cache[key] = entry
        self.current_bytes += entry.size_bytes

    def get(self, stage: int, room: int, cut: int) -> Optional[BackgroundTextureEntry]:
        """Fetch background texture from cache. Returns entry or None if missing."""
        key = f"R{stage:01X}{room:02X}_{cut:02X}"
        with self.lock:
            if key in self.cache:
                self.hits += 1
                entry = self.cache[key]
                self.cache.move_to_end(key)
                entry.last_accessed = time.time()
                return entry

            self.misses += 1

        # Cache miss: synchronous fallback load
        entry = self._load_sync(stage, room, cut)
        return entry
