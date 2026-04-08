from pathlib import Path
import csv


ROUTING_MODE_01 = "01_changetrcak_outframe"
ROUTING_MODE_02 = "02_both"
ROUTING_MODE_03 = "03_outframe_changetrcak"


NETWORK_ATTRIBUTE_TEXT = """default ns3::UbApp::EnableMultiPath "false"
default ns3::UbApp::UseShortestPaths "true"
default ns3::UbLink::Delay "+0ns"
default ns3::UbPort::UbDataRate "400Gbps"
default ns3::UbPort::UbInterframeGap "+0ns"
default ns3::UbPort::CbfcFlitLenByte "20"
default ns3::UbPort::CbfcFlitsPerCell "4"
default ns3::UbPort::CbfcRetCellGrainDataPacket "2"
default ns3::UbPort::CbfcRetCellGrainControlPacket "2"
default ns3::UbPort::CbfcInitCreditCell "1024000"
default ns3::UbPort::PfcUpThld "1677721"
default ns3::UbPort::PfcLowThld "1342176"
default ns3::UbSwitch::FlowControl "CBFC"
default ns3::UbJetty::JettyOooAckThreshold "2048"
default ns3::UbJetty::UbInflightMax "10000"
default ns3::UbTransportChannel::EnableRetrans "false"
default ns3::UbTransportChannel::InitialRTO "+25600ns"
default ns3::UbTransportChannel::MaxRetransAttempts "7"
default ns3::UbTransportChannel::RetransExponentFactor "1"
default ns3::UbTransportChannel::DefaultMaxWqeSegNum "1000"
default ns3::UbTransportChannel::DefaultMaxInflightPacketSize "1000"
default ns3::UbTransportChannel::TpOooThreshold "2048"
default ns3::UbTransportChannel::UsePacketSpray "true"
default ns3::UbTransportChannel::UseShortestPaths "true"
default ns3::UbSwitchAllocator::AllocationTime "+10ns"
default ns3::UbLdstInstance::ThreadNum "10"
default ns3::UbLdstInstance::QueuePriority "1"
default ns3::UbLdstThread::LoadResponseSize "512"
default ns3::UbLdstThread::StoreRequestSize "512"
default ns3::UbLdstThread::LoadRequestSize "64"
default ns3::UbLdstThread::StoreOutstanding "64"
default ns3::UbLdstThread::LoadOutstanding "64"
default ns3::UbLdstApi::UsePacketSpray "true"
default ns3::UbLdstApi::UseShortestPaths "true"
default ns3::UbCaqm::UbCaqmAlpha "0.5"
default ns3::UbCaqm::UbCaqmBeta "0.5"
default ns3::UbCaqm::UbCaqmGamma "0.5"
default ns3::UbCaqm::UbCaqmLambda "0.5"
default ns3::UbCaqm::UbCaqmTheta "10"
default ns3::UbCaqm::UbCaqmQt "40960"
default ns3::UbCaqm::UbCaqmCcUint "32"
default ns3::UbCaqm::UbMarkProbability "0.1"
default ns3::UbHostCaqm::UbCaqmCwnd "40960"
default ns3::UbSwitchCaqm::UbCcUpdatePeriod "+500ns"
default ns3::UbQueueManager::BufferSizePerInportPriority "104857600"
default ns3::UbFault::UbFaultUsePacketSpray "true"

global UB_FAULT_ENABLE "false"
global UB_PRIORITY_NUM "16"
global UB_VL_NUM "16"
global UB_CC_ALGO "CAQM"
global UB_CC_ENABLED "false"
global UB_TRACE_ENABLE "true"
global UB_TASK_TRACE_ENABLE "true"
global UB_PACKET_TRACE_ENABLE "true"
global UB_PORT_TRACE_ENABLE "true"
global UB_RECORD_PKT_TRACE "true"
global UB_PARSE_TRACE_ENABLE "true"
global UB_PYTHON_SCRIPT_PATH "scratch/ns-3-ub-tools/trace_analysis/parse_trace.py"
"""


def ensure_dir(path: str | Path) -> Path:
    path = Path(path)
    path.mkdir(parents=True, exist_ok=True)
    return path


def write_text_lf(path: str | Path, text: str) -> Path:
    path = Path(path)
    path.write_text(text, encoding="utf-8", newline="\n")
    return path


def write_csv_lf(path: str | Path, header: list[str], rows: list[list[object]]) -> Path:
    path = Path(path)
    with path.open("w", encoding="utf-8", newline="") as f:
        writer = csv.writer(f, lineterminator="\n")
        writer.writerow(header)
        writer.writerows(rows)
    return path


def expand_range_spec(spec_text: str) -> tuple[list[str], list[list[object]]]:
    lines = [line.strip() for line in spec_text.strip().splitlines() if line.strip()]
    header = [x.strip() for x in lines[0].split(",")]
    rows: list[list[object]] = []

    for line in lines[1:]:
        parts = [x.strip() for x in line.split(",")]
        node_range = parts[0]
        values = parts[1:]

        start_str, end_str = node_range.split("..")
        start = int(start_str)
        end = int(end_str)

        for node_id in range(start, end + 1):
            row: list[object] = [node_id]
            for value in values:
                if value.isdigit():
                    row.append(int(value))
                else:
                    row.append(value)
            rows.append(row)

    return header, rows


# ============================================================
# network_attribute.txt
# ============================================================

def write_network_attribute(output_dir: str | Path) -> Path:
    output_dir = ensure_dir(output_dir)
    return write_text_lf(output_dir / "network_attribute.txt", NETWORK_ATTRIBUTE_TEXT)


# ============================================================
# node.csv
# nodeId,nodeType,portNum,forwardDelay
# 0..351,DEVICE,1,0ns
# 352..703,SWITCH,24,1ns
# 704..831,SWITCH,44,1ns
# ============================================================

def build_node_rows() -> tuple[list[str], list[list[object]]]:
    rows = []
    rows.append(['0..351', 'DEVICE', 1, '0ns'])
    rows.append(['352..703', 'SWITCH', 24, '1ns'])
    rows.append(['704..831', 'SWITCH', 44, '1ns'])

    headers = ['nodeId', 'nodeType', 'portNum', 'forwardDelay', ]
    return headers, rows


def write_node_csv(output_dir: str | Path) -> Path:
    output_dir = ensure_dir(output_dir)
    header, rows = build_node_rows()
    return write_csv_lf(output_dir / "node.csv", header, rows)


# ============================================================
# topology.csv
# nodeId1,portId1,nodeId2,portId2,bandwidth,delay
# 先写 0..351 -> 352..703
# 再按 352..703 顺序：
#   先组内互联
#   再上联 704..831
# ============================================================

def build_topology_rows() -> list[list[object]]:
    rows: list[list[object]] = []

    # 0..351,0,352..703,0,6000Gbps,0ns
    for device_id in range(352):
        l1_switch_id = 352 + device_id
        rows.append([device_id, 0, l1_switch_id, 0, "6000Gbps", "0ns"])

    # 352..703
    l1_base = 352
    l1_end = 703
    group_size = 8
    l2_base = 704
    l2_block_size = 16

    for l1_switch_id in range(l1_base, l1_end + 1):
        offset = l1_switch_id - l1_base
        group_id = offset // group_size
        pos_in_group = offset % group_size
        group_start = l1_switch_id - pos_in_group

        # 组内互联
        src_port = pos_in_group + 1
        for next_pos in range(pos_in_group + 1, group_size):
            dst_switch_id = group_start + next_pos
            dst_port = pos_in_group + 1
            rows.append([l1_switch_id, src_port, dst_switch_id, dst_port, "400Gbps", "10ns"])
            src_port += 1

        # 上联 704..831
        l2_block_start = l2_base + pos_in_group * l2_block_size
        for k in range(l2_block_size):
            l1_port = 8 + k
            l2_switch_id = l2_block_start + k
            l2_port = group_id
            rows.append([l1_switch_id, l1_port, l2_switch_id, l2_port, "200Gbps", "10ns"])

    return rows


def write_topology_csv(output_dir: str | Path) -> Path:
    output_dir = ensure_dir(output_dir)
    return write_csv_lf(
        output_dir / "topology.csv",
        ["nodeId1", "portId1", "nodeId2", "portId2", "bandwidth", "delay"],
        build_topology_rows(),
    )


# ============================================================
# routing helpers
# ============================================================

def device_frame(device_id: int) -> int:
    return device_id // 8


def device_rail(device_id: int) -> int:
    return device_id % 8


def l1_group(node_id: int) -> int:
    return (node_id - 352) // 8


def l1_pos(node_id: int) -> int:
    return (node_id - 352) % 8


def format_ports(ports: list[int]) -> str:
    return " ".join(str(p) for p in ports)


def format_metrics(n: int) -> str:
    return " ".join(["1"] * n)


def l1_port_to_rail(cur_pos: int, dst_r: int) -> int:
    if dst_r == cur_pos:
        return 0
    if dst_r < cur_pos:
        return dst_r + 1
    return dst_r


# ============================================================
# routing_table.csv
# nodeId,dstNodeId,dstPortId,outPorts,metrics
# 先写 0..351
# 再写 352..703
# 再写 704..831
# ============================================================

def build_device_routing_rows() -> list[list[object]]:
    rows: list[list[object]] = []

    # 0..351
    for src in range(352):
        for dst in range(352):
            if dst == src:
                continue
            rows.append([src, dst, 0, "0", "1"])

    return rows


def outports_mode_01_changetrcak_outframe(node_id: int, dst_device: int) -> tuple[str, str]:
    cur_group = l1_group(node_id)
    cur_pos = l1_pos(node_id)
    dst_f = device_frame(dst_device)
    dst_r = device_rail(dst_device)

    if dst_f == cur_group:
        return str(l1_port_to_rail(cur_pos, dst_r)), "1"

    if dst_r == cur_pos:
        ports = list(range(8, 24))
        return format_ports(ports), format_metrics(len(ports))

    return str(l1_port_to_rail(cur_pos, dst_r)), "1"


def outports_mode_02_both(node_id: int, dst_device: int) -> tuple[str, str]:
    cur_group = l1_group(node_id)
    cur_pos = l1_pos(node_id)
    dst_f = device_frame(dst_device)
    dst_r = device_rail(dst_device)

    rail_port = l1_port_to_rail(cur_pos, dst_r)

    if dst_f == cur_group:
        return str(rail_port), "1"

    if dst_r == cur_pos:
        ports = list(range(8, 24))
        return format_ports(ports), format_metrics(len(ports))

    ports: list[int] = []
    for uplink_port in range(8, 24):
        ports.append(uplink_port)
        ports.append(rail_port)
    return format_ports(ports), format_metrics(len(ports))


def outports_mode_03_outframe_changetrcak(node_id: int, dst_device: int) -> tuple[str, str]:
    cur_group = l1_group(node_id)
    cur_pos = l1_pos(node_id)
    dst_f = device_frame(dst_device)
    dst_r = device_rail(dst_device)

    if dst_f == cur_group:
        return str(l1_port_to_rail(cur_pos, dst_r)), "1"

    ports = list(range(8, 24))
    return format_ports(ports), format_metrics(len(ports))


def build_l1_routing_rows(mode: str) -> list[list[object]]:
    rows: list[list[object]] = []

    # 352..703
    for node_id in range(352, 704):
        for dst in range(352):
            if mode == ROUTING_MODE_01:
                out_ports, metrics = outports_mode_01_changetrcak_outframe(node_id, dst)
            elif mode == ROUTING_MODE_02:
                out_ports, metrics = outports_mode_02_both(node_id, dst)
            elif mode == ROUTING_MODE_03:
                out_ports, metrics = outports_mode_03_outframe_changetrcak(node_id, dst)
            else:
                raise ValueError(f"unsupported routing mode: {mode}")

            rows.append([node_id, dst, 0, out_ports, metrics])

    return rows


def build_l2_routing_rows() -> list[list[object]]:
    rows: list[list[object]] = []

    # 704..831
    # outPort = dst // 8
    for node_id in range(704, 832):
        for dst in range(352):
            out_port = dst // 8
            rows.append([node_id, dst, 0, str(out_port), "1"])

    return rows


def write_routing_table_csv(output_dir: str | Path, mode: str) -> Path:
    output_dir = ensure_dir(output_dir)

    rows: list[list[object]] = []
    rows.extend(build_device_routing_rows())
    rows.extend(build_l1_routing_rows(mode))
    rows.extend(build_l2_routing_rows())

    return write_csv_lf(
        output_dir / "routing_table.csv",
        ["nodeId", "dstNodeId", "dstPortId", "outPorts", "metrics"],
        rows,
    )


# ============================================================
# traffic.csv
# taskId,sourceNode,destNode,dataSize(Byte),opType,priority,delay,phaseId
# 当前按你给的样式:
# 1..351 -> 0, 64512, URMA_WRITE, 7, 10ns, 0
# taskId 先顺序编号
# ============================================================

def build_traffic_rows(
    task_id_start: int = 0,
) -> list[list[object]]:
    rows: list[list[object]] = []
    task_id = task_id_start

    for src in range(1, 352):
        rows.append([
            task_id,
            src,
            0,
            64512,
            "URMA_WRITE",
            7,
            "10ns",
            0,
        ])
        task_id += 1

    return rows


def write_traffic_csv(output_dir: str | Path) -> Path:
    output_dir = ensure_dir(output_dir)
    return write_csv_lf(
        output_dir / "traffic.csv",
        ["taskId", "sourceNode", "destNode", "dataSize(Byte)", "opType", "priority", "delay", "phaseId"],
        build_traffic_rows(),
    )


# ============================================================
# generate case
# ============================================================

def generate_single_case(case_dir: str | Path, routing_mode: str) -> None:
    case_dir = ensure_dir(case_dir)
    write_network_attribute(case_dir)
    write_node_csv(case_dir)
    write_topology_csv(case_dir)
    write_routing_table_csv(case_dir, routing_mode)
    write_traffic_csv(case_dir)


def generate_all_cases(root_dir: str | Path) -> None:
    root_dir = ensure_dir(root_dir)

    modes = [
        ROUTING_MODE_01,
        ROUTING_MODE_02,
        ROUTING_MODE_03,
    ]

    for mode in modes:
        case_dir = root_dir / mode
        generate_single_case(case_dir, mode)
        print(f"generated case: {case_dir}")


if __name__ == "__main__":
    generate_all_cases("scratch/test01")
