import platform
import sys
import struct
import re
import serial
import time
import argparse  # 新增：用于命令行参数解析


def wait_for_download_signal(ser, timeout=30.0):
    """
    监听串口，打印板子调试信息，直到接收到五个连续的 'A' (ASCII 0x41)，表示下载信号。
    使用累积缓冲检查模式，超时后退出。
    """
    print("Please press button and poweron or reset if you want to download")
    print("Waiting for board response... (timeout: {}s)".format(timeout))

    original_timeout = ser.timeout
    ser.timeout = 0.1  # Short timeout for polling
    buffer = b""  # Accumulate incoming data
    start_time = time.time()

    while time.time() - start_time < timeout:
        if ser.in_waiting > 0:
            data = ser.read(ser.in_waiting)
            if data:
                buffer += data
                try:
                    # Decode and print lines (as debug)
                    lines = buffer.decode('utf-8', errors='ignore').splitlines()
                    for line in lines[:-1]:  # All but last (incomplete)
                        stripped = line.strip()
                        if stripped:
                            print(f"*** Kernel Debug: {stripped} ***")
                    # Update buffer to last incomplete line
                    buffer = lines[-1].encode('utf-8') if lines else b""

                    # Check for "AAAAA" in full decoded buffer
                    full_text = buffer.decode('utf-8', errors='ignore')
                    if "AAAAA" in full_text:
                        print("Downloading the program...")
                        ser.timeout = original_timeout  # Restore timeout
                        return True
                except Exception:
                    # Fallback: treat as binary debug, print hex
                    print(f"*** Kernel Debug (hex): {data.hex()} ***")
                    buffer += data  # Keep accumulating for pattern check
                    # Check raw bytes for 5x 0x41 (if not ASCII text)
                    if b'\x41\x41\x41\x41\x41' in buffer:
                        print("Downloading the program...")
                        ser.timeout = original_timeout
                        return True

        time.sleep(0.05)  # Poll every 50ms

    print(f"Timeout waiting for download signal after {timeout}s. Exiting.")
    ser.timeout = original_timeout
    sys.exit(1)  # Or return False if you want to continue without signal

def flush_debug(ser, max_wait=0.2):
    """
    Flush and print any pending debug messages from kernel (assuming ASCII lines ended with \n).
    Wraps kernel debug with *** to distinguish from Python debug.
    """
    original_timeout = ser.timeout
    ser.timeout = 0.05  # Short timeout for quick checks
    start = time.time()
    while time.time() - start < max_wait:
        if ser.in_waiting == 0:
            time.sleep(0.01)
            continue
        data = ser.read(ser.in_waiting or 1)  # Read available bytes
        if data:
            try:
                lines = data.decode('utf-8', errors='ignore').splitlines()
                for line in lines:
                    stripped = line.strip()
                    if stripped:
                        print(f"*** Kernel Debug: {stripped} ***")
            except Exception:
                # Ignore non-text data (e.g., stray binary)
                pass
    ser.timeout = original_timeout

def send_words_batch(ser, words_list, debug_prefix="", verbose=False):
    """
    批量发送 words，但限制每个子批量 <=8 字 (16 字节)，匹配 DSP FIFO 深度。
    大列表自动拆分 + 微延时，提高稳定性和速率。
    """
    if not words_list:
        return

    FIFO_DEPTH_WORDS = 8  # 16 字节 / 2 字节/字
    num_words = len(words_list)

    # Debug print (same as before)
    if debug_prefix or verbose:
        if num_words <= FIFO_DEPTH_WORDS or verbose:
            print(
                f"{debug_prefix}Sending batch: {num_words} words (split into {(num_words + FIFO_DEPTH_WORDS - 1) // FIFO_DEPTH_WORDS} sub-batches)")
        else:
            print(
                f"{debug_prefix}Sending batch: {num_words} words (first: {print_hex(words_list[0])}, last: {print_hex(words_list[-1])})")

    # Split into sub-batches if needed
    for start in range(0, num_words, FIFO_DEPTH_WORDS):
        end = min(start + FIFO_DEPTH_WORDS, num_words)
        sub_batch = words_list[start:end]

        # Pack sub-batch bytes (little-endian)
        bytes_sub = b''.join(struct.pack('<H', word) for word in sub_batch)

        # Debug for sub-batch if verbose
        if verbose and len(sub_batch) < FIFO_DEPTH_WORDS:
            for idx, word in enumerate(sub_batch):
                print(f"  Sub[{start // FIFO_DEPTH_WORDS + 1}][{idx + 1}/{len(sub_batch)}] {print_hex(word)}")

        # Single write for sub-batch
        ser.write(bytes_sub)
        ser.flush()  # Force immediate send
        time.sleep(0.001)  # 1ms per sub-batch (adjust: 0.5ms for faster, 2ms for stability)

    # Optional: final short delay after full batch
    if num_words > FIFO_DEPTH_WORDS:
        time.sleep(0.002)  # Let DSP catch up after multiple sub-batches

def read_word(data, offset):
    """
    Read a 16-bit word (2 bytes) from the data at offset.
    Assumes little-endian byte order: LSB first, then MSB.
    Returns the integer value.
    """
    if offset + 2 > len(data):
        raise ValueError("Insufficient data for word at offset {}".format(offset))
    bytes_le = data[offset:offset + 2]
    # Unpack as little-endian unsigned short
    value = struct.unpack('<H', bytes_le)[0]
    return value


def read_long(data, offset):
    """
    Read a 32-bit long (4 bytes, two words) from the data at offset.
    Each word is little-endian bytes, but words are combined big-endian: high_word << 16 | low_word.
    Returns the integer value.
    """
    if offset + 4 > len(data):
        raise ValueError("Insufficient data for long at offset {}".format(offset))
    high_word = read_word(data, offset)
    low_word = read_word(data, offset + 2)
    return (high_word << 16) | low_word


def print_hex(value, width=4):
    """
    Print a value in hex format with leading zeros, e.g., 0x000A.
    """
    return "0x{:0{}X}".format(value, width)


def calculate_checksum(words):
    """
    Calculate 16-bit checksum: for each word, add low 8 bits + high 8 bits, accumulate.
    Starts from 0.
    """
    checksum = 0
    for word in words:
        checksum += (word & 0xFF) + ((word >> 8) & 0xFF)
    return checksum & 0xFFFF  # Keep 16 bits


def send_word(ser, word, debug_prefix="", verbose=False):
    """
    Send a 16-bit word as 2 bytes (little-endian) over serial.
    Prints debug info only if verbose=True or prefix provided.
    """
    if debug_prefix or verbose:
        byte_str = struct.pack('<H', word).hex()
        print(f"{debug_prefix}Sending word: {print_hex(word)} (bytes: {byte_str})")
    bytes_le = struct.pack('<H', word)
    ser.write(bytes_le)
    time.sleep(0.001)  # Small delay for stability


def send_long(ser, addr, debug_prefix="", verbose=False):
    """Send 32-bit addr as batch of two words (high first)."""
    high_word = (addr >> 16) & 0xFFFF
    low_word = addr & 0xFFFF
    send_words_batch(ser, [high_word, low_word], debug_prefix + "Addr: ", verbose)


def receive_checksum(ser, timeout=2.0):
    """
    ... (docstring unchanged)
    """
    original_timeout = ser.timeout
    ser.timeout = 0.1  # Short polling for marker detection
    buffer = b""  # Accumulate incoming data
    marker = b"CheckSum send"
    start_time = time.time()
    marker_found = False

    # Wait for marker with timeout
    while time.time() - start_time < timeout:
        if ser.in_waiting > 0:
            data = ser.read(ser.in_waiting)
            if data:
                buffer += data
                # 先 raw 检查 marker（优先，避免 decode 干扰二进制）
                if marker in buffer:
                    # 打印到 marker 前的数据（模拟 flush）
                    pre_marker = buffer.split(marker, 1)[0]
                    if pre_marker:
                        try:
                            lines = pre_marker.decode('utf-8', errors='ignore').splitlines()
                            for line in lines:
                                stripped = line.strip()
                                if stripped:
                                    print(f"*** Kernel Debug: {stripped} ***")
                        except:
                            pass
                    print("*** Kernel Debug: CheckSum send ***")
                    # 移除 marker，保留后续 buffer（现在用于提取 checksum）
                    post_marker = buffer.split(marker, 1)[1]
                    marker_found = True
                    break  # 立即 break

                # # 如果无 marker，继续 decode/print 调试（限小数据）
                # try:
                #     lines = buffer.decode('utf-8', errors='ignore').splitlines()
                #     for line in lines[:-1]:
                #         stripped = line.strip()
                #         if stripped:
                #             print(f"*** Kernel Debug: {stripped} ***")
                #     buffer = lines[-1].encode('utf-8') if lines else b""
                # except Exception:
                #     buffer += data  # fallback 保持 raw
                try:
                    full_text = buffer.decode('utf-8', errors='ignore')
                    lines = full_text.splitlines(keepends=False)  # splitlines，但不区分完整/不完整，直接取所有行
                    for line in lines:  # 遍历所有行（忽略[:-1]，直接打印全部）
                        stripped = line.strip()
                        if stripped:
                            print(f"*** Kernel Debug: {stripped} ***")
                    buffer = b""  # 清空 buffer（忽略不完整行，假设每次都完整处理）
                except Exception:
                    buffer += data  # fallback 保持 raw

        time.sleep(0.05)  # Poll every 50ms

    if not marker_found:
        raise ValueError(f"Checksum marker 'CheckSum send' not received within {timeout}s")

    # 从 post_marker 或 ser 提取 checksum（处理包含/分发情况）
    checksum_data = b""
    if len(post_marker) >= 2:
        # 如果 buffer 已含 checksum，直接取（最常见）
        checksum_data = post_marker[:2]
    else:
        # 如果分发，短读 ser 补齐（e.g., marker 单独一轮）
        ser.timeout = 0.5  # 短超时
        try:
            remaining = ser.read(2 - len(post_marker))  # 补剩余字节
            checksum_data += remaining
        except serial.SerialTimeoutException:
            pass  # 如果超时，checksum_data 可能短

    ser.timeout = timeout  # 恢复长超时备用
    if len(checksum_data) < 2:
        # 最终补读（防极慢）
        checksum_data += ser.read(2 - len(checksum_data))

    if len(checksum_data) < 2:
        raise ValueError(f"数据太短：期望 2 字节，收到 {len(checksum_data)} 字节 (got: {checksum_data.hex()})")

    # 解包
    checksum = struct.unpack('<H', checksum_data)[0]
    print(f"  Received checksum: {print_hex(checksum, 4)}")  # 可选：移到调用处

    # ... (except/finally unchanged, 但移除旧 ser.read(2))
    return checksum

def send_and_verify_block(ser, words, block_desc, max_retries=5, timeout=2.0):
    """
    Send a block of words as BATCH, calculate checksum, wait for DSP checksum, verify.
    Retry up to max_retries. Now uses batch sending for speed.
    """
    for attempt in range(1, max_retries + 1):
        print(f"\n--- {block_desc} Attempt {attempt}/{max_retries} ---")

        # Batch send all words
        send_words_batch(ser, words, f"  Batch sending {block_desc}: ", verbose=(len(words) <= 10))

        # Calculate PC checksum (unchanged)
        checksum = calculate_checksum(words)
        print(f"  Calculated PC checksum (for verification): {print_hex(checksum, 4)}")

        # Receive and verify DSP checksum (unchanged)
        try:
            recv_checksum = receive_checksum(ser, timeout)
            print(f"  Received checksum in {timeout}s timeout window: {print_hex(recv_checksum, 4)}")

            if recv_checksum == checksum:
                print(f"  ✓ {block_desc} verified OK!")
                return True
            else:
                print(f"  ✗ Checksum mismatch! Expected {print_hex(checksum, 4)}, got {print_hex(recv_checksum, 4)}")
        except ValueError as e:
            print(f"  ✗ Receive error (timeout={timeout}s): {e}")

        if attempt < max_retries:
            print(f"  Retrying in 0.5s...")
            time.sleep(0.5)

    print(f"\n✗ Failed to verify {block_desc} after {max_retries} retries. Exiting program.")
    sys.exit(1)


def send_and_verify_data_block(ser, block_data_words, dest_addr, block_size, block_desc, buffer_len=0x400, max_chunk_retries=5, timeout=45.0):
    """
    Send large block with chunking, but each chunk as BATCH send.
    First chunk: size + addr + data[0:buffer_len]; others: data only.
    """
    if block_size == 0:
        print(f"  End block (size=0)")
        send_word(ser, 0)  # Keep single send for end marker if needed
        return True

    size_word = block_size & 0xFFFF
    addr_high = (dest_addr >> 16) & 0xFFFF
    addr_low = dest_addr & 0xFFFF

    # Send size and addr as small batch
    print(f"\n--- {block_desc} (size={block_size}, addr=0x{dest_addr:08X}) ---")
    send_words_batch(ser, [size_word], "  Batch size: ", verbose=True)
    send_long(ser, dest_addr, "  Batch addr: ", verbose=True)

    num_full_chunks = block_size // buffer_len
    remainder = block_size % buffer_len

    # Full chunks
    for j in range(num_full_chunks):
        chunk_start = j * buffer_len
        chunk_words = block_data_words[chunk_start:chunk_start + buffer_len]

        chunk_success = False
        for chunk_attempt in range(1, max_chunk_retries + 1):
            print(f"    --- Full chunk {j+1}/{num_full_chunks} Attempt {chunk_attempt}/{max_chunk_retries} ---")

            # Batch send chunk data
            send_words_batch(ser, chunk_words, f"      Batch full chunk {j+1}: ", verbose=(len(chunk_words) <= 10))

            # Calculate checksum for this chunk (unchanged)
            if j == 0:
                chunk_checksum = calculate_checksum([size_word, addr_high, addr_low] + chunk_words)
            else:
                chunk_checksum = calculate_checksum(chunk_words)

            # Verify (unchanged)
            recv_checksum = receive_checksum(ser, timeout)
            print(f"      Received checksum: {print_hex(recv_checksum, 4)} (expected: {print_hex(chunk_checksum, 4)})")

            if recv_checksum == chunk_checksum:
                print(f"      ✓ Full chunk {j+1} verified!")
                chunk_success = True
                break
            else:
                print(f"      ✗ Full chunk {j+1} mismatch!")
                if chunk_attempt < max_chunk_retries:
                    print(f"      Retrying chunk in 0.5s...")
                    time.sleep(0.5)
                else:
                    break

        if not chunk_success:
            print(f"\n✗ Failed {block_desc} after chunk failures. Exiting.")
            sys.exit(1)

    # Remainder chunk (similar batch logic)
    if remainder > 0:
        chunk_start = num_full_chunks * buffer_len
        chunk_words = block_data_words[chunk_start:]

        chunk_success = False
        for chunk_attempt in range(1, max_chunk_retries + 1):
            print(f"    --- Remainder Chunk Attempt {chunk_attempt}/{max_chunk_retries} ---")

            # Batch send remainder
            send_words_batch(ser, chunk_words, f"      Batch remainder: ", verbose=(len(chunk_words) <= 10))

            # Calculate checksum
            if num_full_chunks == 0:
                chunk_checksum = calculate_checksum([size_word, addr_high, addr_low] + chunk_words)
            else:
                chunk_checksum = calculate_checksum(chunk_words)

            # Verify
            recv_checksum = receive_checksum(ser, timeout)
            print(f"      Received checksum: {print_hex(recv_checksum, 4)} (expected: {print_hex(chunk_checksum, 4)})")

            if recv_checksum == chunk_checksum:
                print(f"      ✓ Remainder chunk verified!")
                chunk_success = True
                break
            else:
                print(f"      ✗ Remainder chunk mismatch!")
                if chunk_attempt < max_chunk_retries:
                    print(f"      Retrying remainder in 0.5s...")
                    time.sleep(0.5)
                else:
                    break

        if not chunk_success:
            print(f"\n✗ Failed {block_desc} after remainder failure. Exiting.")
            sys.exit(1)

    print(f"  ✓ {block_desc} full block verified!")
    return True


def parse_file_info(filename):
    """
    解析文件，只提取块信息（总块数、每个块的起始地址和大小），不发送串口。
    返回块列表：[(addr, size), ...]
    """
    with open(filename, 'r') as f:
        content = f.read().upper()

    hex_str = re.sub(r'[^0-9A-F]', '', content)
    if len(hex_str) % 2 != 0:
        raise ValueError("Hex string length must be even after cleaning")

    raw_data = bytes.fromhex(hex_str)

    i = 0
    if len(raw_data) >= 2 and raw_data[0] == 0x02 and raw_data[1] == 0x0A:
        i = 2
        print("Skipped STX + newline at start")

    data = raw_data[i:]

    if data and data[-1] == 0x03:
        data = data[:-1]
        print("Removed ETX at end")

    print(f"Processed file size: {len(data)} bytes ({len(data) // 2} words)")

    # Skip first block header (not data block)
    i += 2  # header
    i += 16  # 8 reserved words (16 bytes)
    i += 4   # entry addr

    # Extract data blocks
    blocks = []
    block_count = 0
    while i < len(data):
        block_size = read_word(data, i)
        i += 2
        if block_size == 0:
            break

        dest_addr = read_long(data, i)
        i += 4

        # Skip data words (don't read them for -l mode)
        i += block_size * 2

        blocks.append((dest_addr, block_size))
        block_count += 1

    return blocks, block_count


def print_block_info(blocks, total_blocks):
    """
    打印块信息：总段数（假设每个 block 为一“段”），每段块数（这里 size 为字数），起始地址。
    """
    print(f"\n=== File Block Information ===")
    print(f"Total number of segments/blocks: {total_blocks}")
    for idx, (addr, size) in enumerate(blocks, 1):
        print(f"Segment {idx}: {size} words, starting address {print_hex(addr, 8)}")

def main():
    # 命令行参数解析
    parser = argparse.ArgumentParser(description="Parse and send .txt file to DSP via serial.")
    parser.add_argument("filename", help="Input .txt file (e.g., AIC32_codec.txt)", default="AIC32_codec.txt")
    parser.add_argument("-w", "--write", action="store_true", help="Start serial transmission (requires --port)")
    parser.add_argument("-l", "--list", action="store_true", help="List block info only (total segments, per segment blocks and start addr), no transmission")
    parser.add_argument("-p", "--port", help="Serial port (default: COM4)")
    args = parser.parse_args()

    filename = args.filename
    do_write = args.write
    do_list = args.list

    # 新增：动态设置默认端口，实现 Windows/Linux 兼容
    if args.port is None:
        os_name = platform.system().lower()
        if os_name == "windows":
            args.port = "COM4"
            print("Detected Windows: Using default port COM4")
        elif os_name == "linux":
            args.port = "/dev/ttyUSB0"  # 常见 Linux USB 串口；用户可覆盖
            print("Detected Linux: Using default port /dev/ttyUSB0")
            print(
                "Note: Ensure user is in 'dialout' group for serial access (sudo usermod -a -G dialout $USER; logout/login)")
        else:
            print(
                f"Unsupported OS: {os_name}. Please specify --port manually (e.g., COM4 for Windows, /dev/ttyUSB0 for Linux)")
            sys.exit(1)
    else:
        print(f"Using specified port: {args.port}")

    port = args.port

    if do_list:
        # 只解析并打印信息
        blocks, total_blocks = parse_file_info(filename)
        print_block_info(blocks, total_blocks)
        return

    if do_write:
        # 执行串口传输（原 main 逻辑，移入此处）
        # Parse file
        with open(filename, 'r') as f:
            content = f.read().upper()

        hex_str = re.sub(r'[^0-9A-F]', '', content)
        if len(hex_str) % 2 != 0:
            raise ValueError("Hex string length must be even after cleaning")

        raw_data = bytes.fromhex(hex_str)

        i = 0
        if len(raw_data) >= 2 and raw_data[0] == 0x02 and raw_data[1] == 0x0A:
            i = 2
            print("Skipped STX + newline at start")

        data = raw_data[i:]

        if data and data[-1] == 0x03:
            data = data[:-1]
            print("Removed ETX at end")

        print(f"Processed file size: {len(data)} bytes ({len(data) // 2} words)")

        # Print first 10 words for verification
        print("\nFirst 10 words (little-endian, hex):")
        for j in range(min(10, len(data) // 2)):
            word = read_word(data, j * 2)
            print(f"Word {j + 1}: {print_hex(word)}")

        # Open serial port with write timeout
        try:
            ser = serial.Serial(port, baudrate=921600, bytesize=8, parity='N', stopbits=1, timeout=1)
            ser.write_timeout = 0.5  # Prevent long blocks from hanging write
            print(f"Opened serial port: {port} @ 921600 8N1")
            time.sleep(1)  # Allow settling
            wait_for_download_signal(ser)  # Wait for download signal before proceeding
        except serial.SerialException as e:
            print(f"Failed to open serial port {port}: {e}")
            if platform.system().lower() == "linux" and "Permission denied" in str(e):
                print("Linux tip: Run 'sudo chmod 666 /dev/ttyUSB0' or add user to dialout group.")
            sys.exit(1)

        # Full parsing and sending
        i = 0
        flash_addresses = []

        # First block
        print("\n=== Preparing First Block ===")
        header = read_word(data, i)
        i += 2
        print(f"Header: {print_hex(header)}")

        reserved_words = []
        for j in range(8):
            res_word = read_word(data, i)
            reserved_words.append(res_word)
            i += 2
        print(f"Reserved (8 words): {[print_hex(w) for w in reserved_words]}")

        entry_addr = read_long(data, i)
        i += 4
        print(f"Entry Address: {print_hex(entry_addr, 8)}")

        # Prepare first block words: header + reserved + entry (high word first, then low)
        entry_high = (entry_addr >> 16) & 0xFFFF
        entry_low = entry_addr & 0xFFFF
        first_block_words = [header] + reserved_words + [entry_high, entry_low]
        print(f"First block total words: {len(first_block_words)}")

        # Send and verify first block (small block, use original function)
        send_and_verify_block(ser, first_block_words, "First Block", 5, 45)

        # Subsequent blocks
        block_count = 0
        PROG_BUFFER_LENGTH = 0x800
        while i < len(data):
            block_count += 1
            print(f"\n=== Preparing Block {block_count} ===")

            block_size = read_word(data, i)
            i += 2
            print(f"Block Size (words): {print_hex(block_size)} ({block_size} words, {block_size * 2} bytes)")

            if block_size == 0:
                print("End of data (block size 0)")
                send_word(ser, 0)  # Send end block
                break

            dest_addr = read_long(data, i)
            i += 4
            print(f"Destination Address: {print_hex(dest_addr, 8)}")
            flash_addresses.append(dest_addr)

            # Read block data words
            block_data_words = []
            for j in range(block_size):
                word = read_word(data, i)
                block_data_words.append(word)
                i += 2

            print(f"Block {block_count} data words loaded: {len(block_data_words)}")

            # Send and verify block (use chunked function for large blocks)
            if block_size <= PROG_BUFFER_LENGTH:
                # Small block: size + dest + data
                dest_high = (dest_addr >> 16) & 0xFFFF
                dest_low = dest_addr & 0xFFFF
                block_words = [block_size, dest_high, dest_low] + block_data_words
                send_and_verify_block(ser, block_words, f"Small Block {block_count}", 5, 45)
            else:
                # Large block: use chunked sending
                send_and_verify_data_block(ser, block_data_words, dest_addr, block_size, f"Large Block {block_count}", PROG_BUFFER_LENGTH, 5, 45)

        if i != len(data):
            print(f"\nWarning: {len(data) - i} bytes remaining after parsing.")

        # # Summary
        # print(f"\n=== Summary of All Flash Write Addresses ({len(flash_addresses)} blocks) ===")
        # for idx, addr in enumerate(flash_addresses, 1):
        #     print(f"Block {idx}: {print_hex(addr, 8)}")
        #
        # ser.close()
        # print("Serial communication completed successfully.")
        # Summary
        print(f"\n=== Summary of All Flash Write Addresses ({len(flash_addresses)} blocks) ===")
        for idx, addr in enumerate(flash_addresses, 1):
            print(f"Block {idx}: {print_hex(addr, 8)}")

        # 新增：等待并打印 DSP 烧写成功后的调试信息
        print("\nWaiting for DSP final confirmation (success messages)...")
        flush_debug(ser, 2.0)  # 等待最多 2s，打印任何剩余调试（如 "Burn success"）

        ser.close()
        print("Serial communication completed successfully.")
    else:
        # 默认不传参数时，打印帮助
        parser.print_help()


if __name__ == "__main__":
    main()  # 修改：无参数调用