# PTT SPI / UDP Protocol

本文档是 DSP 与 Art-Pi 两端 PTT 协议常量的唯一说明来源。修改协议数值或字段含义时，必须同步检查：

- DSP: `lib/codec_service_lib/codec_service_ptt_spi.c`
- Art-Pi: `applications/ptt_protocol.h`

## SPI Block Header

DSP 与 Art-Pi 的 SPI block 使用 16-bit word 作为基本单位，固定 8-word header。

| Word | 字段 | 说明 |
|---|---|---|
| 0 | magic | 固定 `0x5054` |
| 1 | version | 固定 `0x0001` |
| 2 | header_words | 固定 `8` |
| 3 | block_type | SPI block 类型 |
| 4 | session_id | PTT session 编号 |
| 5 | block_id / state | 上传/下载时为 block_id；状态包中可表示 Art-Pi state |
| 6 | block_count / ack_block | 上传/下载时为总 block 数；状态包中表示被 ACK 的 block |
| 7 | payload_bytes / status | 数据包中为有效 payload byte 数；状态包中为 status code |

### SPI Block Type

| 数值 | Art-Pi 名称 | DSP 镜像名称 | 含义 |
|---|---|---|---|
| 1 | `PTT_SPI_BLOCK_DSP_UPLOAD_BLOCK` | `CODEC_SERVICE_PTT_SPI_BLOCK_DSP_UPLOAD_BLOCK` | DSP 上传一块 AMR 数据 |
| 2 | `PTT_SPI_BLOCK_DSP_DOWNLOAD_REQ` | `CODEC_SERVICE_PTT_SPI_BLOCK_DSP_DOWNLOAD_REQ` | DSP 请求下载远端 AMR 下一块 |
| 3 | `PTT_SPI_BLOCK_ARTPI_DOWNLOAD_BLOCK` | `CODEC_SERVICE_PTT_SPI_BLOCK_ARTPI_DOWNLOAD_BLOCK` | Art-Pi 返回一块远端 AMR 数据 |
| 4 | `PTT_SPI_BLOCK_ARTPI_NO_SESSION` | `CODEC_SERVICE_PTT_SPI_BLOCK_ARTPI_NO_SESSION` | Art-Pi 当前没有可下载 session |
| 5 | `PTT_SPI_BLOCK_ARTPI_STATUS` | `CODEC_SERVICE_PTT_SPI_BLOCK_ARTPI_STATUS` | Art-Pi 返回上一操作状态 |
| 6 | `PTT_SPI_BLOCK_DSP_STATUS_POLL` | `CODEC_SERVICE_PTT_SPI_BLOCK_DSP_STATUS_POLL` | DSP 查询上一操作状态 |
| 7 | `PTT_SPI_BLOCK_DSP_FLOOR_REQUEST` | `CODEC_SERVICE_PTT_SPI_BLOCK_DSP_FLOOR_REQUEST` | DSP 申请话权 |
| 8 | `PTT_SPI_BLOCK_DSP_FLOOR_RELEASE` | `CODEC_SERVICE_PTT_SPI_BLOCK_DSP_FLOOR_RELEASE` | DSP 释放话权 |
| 9 | `PTT_SPI_BLOCK_ARTPI_FLOOR_GRANTED` | `CODEC_SERVICE_PTT_SPI_BLOCK_ARTPI_FLOOR_GRANTED` | Art-Pi 通知 DSP 话权授权 |
| 10 | `PTT_SPI_BLOCK_ARTPI_FLOOR_DENIED` | `CODEC_SERVICE_PTT_SPI_BLOCK_ARTPI_FLOOR_DENIED` | Art-Pi 通知 DSP 话权拒绝 |

### SPI Status

| 数值 | Art-Pi 名称 | 含义 |
|---|---|---|
| 0 | `PTT_SPI_STATUS_OK` | 处理成功 |
| 1 | `PTT_SPI_STATUS_ERR_HEADER` | header/magic/version/type 错误 |
| 2 | `PTT_SPI_STATUS_ERR_LENGTH` | 长度错误 |
| 3 | `PTT_SPI_STATUS_ERR_ORDER` | block 顺序错误 |
| 4 | `PTT_SPI_STATUS_ERR_CAPACITY` | 缓冲区容量不足 |
| 5 | `PTT_SPI_STATUS_ERR_BUSY` | 当前忙或话权冲突 |

## SPI Payload Packing

SPI payload 仍以 16-bit word 传输，但 `payload_bytes` 表示逻辑有效字节数。

- payload byte 0 放在 word 的 `[15:8]`
- payload byte 1 放在 word 的 `[7:0]`
- `payload_words = (payload_bytes + 1) / 2`
- payload 为奇数字节时，最后一个 word 的低 8 bit 补 `0`

## UDP Packet Header

Art-Pi 与 Art-Pi 之间的 UDP 是 byte stream，所有多字节字段使用大端序。固定 18-byte header。

| Byte Offset | 字段 | 长度 | 说明 |
|---|---|---|---|
| 0 | magic | 2 | 固定 `0x5055` |
| 2 | version | 2 | 固定 `0x0001` |
| 4 | packet_type | 2 | UDP packet 类型 |
| 6 | session_id | 2 | PTT session 编号 |
| 8 | chunk_id | 2 | DATA/ACK chunk 编号 |
| 10 | chunk_count | 2 | DATA 总 chunk 数 |
| 12 | payload_bytes / status | 2 | DATA 中为 payload 长度；ACK/控制结果中可复用为 status |
| 14 | total_bytes | 4 | 整段 AMR 总 byte 数 |

### UDP Packet Type

| 数值 | 名称 | 含义 |
|---|---|---|
| 1 | `PTT_UDP_PACKET_SESSION_DATA` | 语音 session 数据 chunk |
| 2 | `PTT_UDP_PACKET_SESSION_ACK` | DATA chunk ACK |
| 3 | `PTT_UDP_PACKET_FLOOR_REQUEST` | 话权申请 |
| 4 | `PTT_UDP_PACKET_FLOOR_GRANTED` | 话权授权 |
| 5 | `PTT_UDP_PACKET_FLOOR_DENIED` | 话权拒绝 |
| 6 | `PTT_UDP_PACKET_FLOOR_RELEASE` | 话权释放 |

### UDP Status

| 数值 | 名称 | 含义 |
|---|---|---|
| 0 | `PTT_UDP_STATUS_OK` | 处理成功 |
| 1 | `PTT_UDP_STATUS_ERR_HEADER` | header/magic/version/type 错误 |
| 2 | `PTT_UDP_STATUS_ERR_LENGTH` | 长度错误 |
| 3 | `PTT_UDP_STATUS_ERR_ORDER` | chunk 顺序错误或话权冲突 |
| 4 | `PTT_UDP_STATUS_ERR_CAPACITY` | 接收缓存容量不足 |

## Main Flow Names

### 本地讲话，上行链路

```text
DSP key press
 -> DSP_FLOOR_REQUEST
 -> Art-Pi UDP FLOOR_REQUEST
 -> FLOOR_GRANTED
 -> DSP record / AMR encode
 -> DSP_UPLOAD_BLOCK...
 -> Art-Pi UDP SESSION_DATA...
 -> FLOOR_RELEASE
```

建议代码命名使用 `floor/local`、`uplink`、`send_uplink_session` 表达方向。

### 远端讲话，下行链路

```text
UDP FLOOR_REQUEST
 -> grant / deny
 -> UDP SESSION_DATA...
 -> downlink AMR complete
 -> DATA_READY high
 -> DSP_DOWNLOAD_REQ...
 -> DSP decode / play
```

建议代码命名使用 `remote floor`、`downlink`、`prepare_download_block`、`consume_by_dsp` 表达方向。
