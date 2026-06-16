#!/usr/bin/env python3
"""Convert a Keras model to TensorFlow Lite for STM32Cube.AI import.

Examples:
    python tools/keras_to_tflite.py
    python tools/keras_to_tflite.py --input tools/best_esc50_model.keras
    python tools/keras_to_tflite.py --quantization dynamic
    python tools/keras_to_tflite.py --quantization int8 --representative-npy tools/calibration.npy
"""

from __future__ import annotations

import argparse
import sys
from pathlib import Path
from typing import Iterable

import numpy as np


DEFAULT_MODEL = Path(__file__).with_name("best_esc50_model.keras")


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(
        description="Convert a .keras model into a .tflite file for STM32Cube.AI."
    )
    parser.add_argument(
        "--input",
        "-i",
        type=Path,
        default=DEFAULT_MODEL,
        help=f"Path to the input .keras model. Default: {DEFAULT_MODEL}",
    )
    parser.add_argument(
        "--output",
        "-o",
        type=Path,
        help="Path to the output .tflite file. Default: same name as input.",
    )
    parser.add_argument(
        "--quantization",
        "-q",
        choices=("none", "dynamic", "float16", "int8"),
        default="none",
        help=(
            "Quantization mode. Use 'none' first for compatibility checks, "
            "then 'dynamic' or 'int8' to reduce model size."
        ),
    )
    parser.add_argument(
        "--representative-npy",
        type=Path,
        help=(
            "NumPy .npy file with representative input samples. Required for "
            "--quantization int8. Shape should be [num_samples, ...input_shape]."
        ),
    )
    parser.add_argument(
        "--inference-input-type",
        choices=("float32", "int8", "uint8"),
        default="int8",
        help="Input tensor type for full-int8 conversion. Default: int8.",
    )
    parser.add_argument(
        "--inference-output-type",
        choices=("float32", "int8", "uint8"),
        default="int8",
        help="Output tensor type for full-int8 conversion. Default: int8.",
    )
    return parser.parse_args()


def require_tensorflow():
    try:
        import tensorflow as tf
    except ImportError as exc:
        raise SystemExit(
            "TensorFlow is not installed in this Python environment.\n"
            "Install it with:\n"
            "  python -m pip install tensorflow\n"
        ) from exc

    return tf


def representative_dataset(samples: np.ndarray) -> Iterable[list[np.ndarray]]:
    for sample in samples:
        yield [np.expand_dims(sample.astype(np.float32), axis=0)]


def configure_converter(tf, converter, args: argparse.Namespace) -> None:
    converter.target_spec.supported_ops = [tf.lite.OpsSet.TFLITE_BUILTINS]

    if args.quantization == "none":
        return

    converter.optimizations = [tf.lite.Optimize.DEFAULT]

    if args.quantization == "float16":
        converter.target_spec.supported_types = [tf.float16]
        return

    if args.quantization == "dynamic":
        return

    if args.quantization == "int8":
        if args.representative_npy is None:
            raise SystemExit(
                "--representative-npy is required when using --quantization int8."
            )
        if not args.representative_npy.exists():
            raise SystemExit(f"Representative dataset not found: {args.representative_npy}")

        samples = np.load(args.representative_npy)
        if samples.ndim < 2:
            raise SystemExit(
                "Representative dataset must include a sample dimension, "
                "for example [num_samples, ...input_shape]."
            )

        converter.representative_dataset = lambda: representative_dataset(samples)
        converter.inference_input_type = getattr(tf, args.inference_input_type)
        converter.inference_output_type = getattr(tf, args.inference_output_type)


def print_tflite_summary(tf, tflite_path: Path) -> None:
    interpreter = tf.lite.Interpreter(model_path=str(tflite_path))
    interpreter.allocate_tensors()

    print("\nTFLite model summary")
    print("--------------------")
    print(f"Path: {tflite_path}")
    print(f"Size: {tflite_path.stat().st_size / 1024:.1f} KiB")

    print("\nInputs:")
    for item in interpreter.get_input_details():
        print(f"  {item['name']}: shape={item['shape'].tolist()} dtype={item['dtype']}")

    print("\nOutputs:")
    for item in interpreter.get_output_details():
        print(f"  {item['name']}: shape={item['shape'].tolist()} dtype={item['dtype']}")


def main() -> int:
    args = parse_args()
    input_path = args.input.resolve()
    output_path = (args.output or input_path.with_suffix(".tflite")).resolve()

    if input_path.suffix.lower() != ".keras":
        raise SystemExit(f"Input must be a .keras file: {input_path}")
    if not input_path.exists():
        raise SystemExit(f"Input model not found: {input_path}")

    output_path.parent.mkdir(parents=True, exist_ok=True)

    tf = require_tensorflow()
    print(f"Loading Keras model: {input_path}")
    model = tf.keras.models.load_model(input_path, compile=False)

    print(f"Converting to TFLite ({args.quantization})...")
    converter = tf.lite.TFLiteConverter.from_keras_model(model)
    configure_converter(tf, converter, args)

    tflite_model = converter.convert()
    output_path.write_bytes(tflite_model)

    print_tflite_summary(tf, output_path)
    print("\nDone. Import this .tflite file in STM32Cube.AI.")
    return 0


if __name__ == "__main__":
    sys.exit(main())
