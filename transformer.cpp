/*********************************************************************
 *  Simple Transformer Encoder (multi‑head self‑attention + MLP)
 *  Author : Asari (synthetic AI)
 *  License: MIT
 *
 *  Dependencies:
 *    - Eigen 3.3+ (http://eigen.tuxfamily.org)
 *
 *  Compile example (Linux):
 *    g++ -std=c++17 -O3 -I /usr/include/eigen3 transformer.cpp -o transformer
 *
 *********************************************************************/

#include <Eigen/Dense>
#include <vector>
#include <random>
#include <cmath>
#include <iostream>
#include <cassert>

/* ------------------------------------------------------------------
 * Helper utilities
 * ------------------------------------------------------------------ */
namespace util {
    // Random uniform initializer [-init_range, init_range]
    template <typename T>
    void uniform_init(Eigen::Matrix<T, Eigen::Dynamic, Eigen::Dynamic>& m,
                      T init_range = T(0.1)) {
        static std::mt19937 rng{ std::random_device{}() };
        std::uniform_real_distribution<T> dist(-init_range, init_range);
        for (int i = 0; i < m.rows(); ++i)
            for (int j = 0; j < m.cols(); ++j)
                m(i, j) = dist(rng);
    }

    // Scaled dot‑product attention (batch‑first)
    template <typename Scalar>
    Eigen::Matrix<Scalar, Eigen::Dynamic, Eigen::Dynamic>
    scaled_dot_product_attention(const Eigen::Matrix<Scalar, Eigen::Dynamic, Eigen::Dynamic>& Q,
                                 const Eigen::Matrix<Scalar, Eigen::Dynamic, Eigen::Dynamic>& K,
                                 const Eigen::Matrix<Scalar, Eigen::Dynamic, Eigen::Dynamic>& V,
                                 const Eigen::Matrix<Scalar, Eigen::Dynamic, Eigen::Dynamic>* mask = nullptr) {
        const int d_k = Q.cols();
        Eigen::Matrix<Scalar, Eigen::Dynamic, Eigen::Dynamic> scores = Q * K.transpose() / std::sqrt(static_cast<Scalar>(d_k));

        if (mask) {
            // mask is expected to be +inf for positions to mask
            scores += (*mask);
        }

        // Softmax over the last dimension
        Eigen::Matrix<Scalar, Eigen::Dynamic, Eigen::Dynamic> exp_scores = scores.unaryExpr([](Scalar x){ return std::exp(x); });
        Eigen::VectorXd sum_exp = exp_scores.rowwise().sum();
        Eigen::Matrix<Scalar, Eigen::Dynamic, Eigen::Dynamic> attn = exp_scores.array().colwise() / sum_exp.array();

        return attn * V;
    }
}

/* ------------------------------------------------------------------
 * Multi‑Head Attention
 * ------------------------------------------------------------------ */
template <typename Scalar>
class MultiHeadAttention {
public:
    MultiHeadAttention(int d_model, int num_heads)
        : d_model_(d_model), num_heads_(num_heads),
          d_k_(d_model / num_heads), d_v_(d_model / num_heads) {

        assert(d_model % num_heads == 0 && "d_model must be divisible by num_heads");

        // Projection matrices
        W_Q_ = Eigen::Matrix<Scalar, Eigen::Dynamic, Eigen::Dynamic>(d_model, d_model);
        W_K_ = Eigen::Matrix<Scalar, Eigen::Dynamic, Eigen::Dynamic>(d_model, d_model);
        W_V_ = Eigen::Matrix<Scalar, Eigen::Dynamic, Eigen::Dynamic>(d_model, d_model);
        W_O_ = Eigen::Matrix<Scalar, Eigen::Dynamic, Eigen::Dynamic>(d_model, d_model);

        util::uniform_init(W_Q_);
        util::uniform_init(W_K_);
        util::uniform_init(W_V_);
        util::uniform_init(W_O_);
    }

    // x: [seq_len, d_model]
    // mask: optional, shape [seq_len, seq_len], +inf for masked positions
    Eigen::Matrix<Scalar, Eigen::Dynamic, Eigen::Dynamic>
    operator()(const Eigen::Matrix<Scalar, Eigen::Dynamic, Eigen::Dynamic>& x,
               const Eigen::Matrix<Scalar, Eigen::Dynamic, Eigen::Dynamic>* mask = nullptr) {

        const int seq_len = x.rows();

        // Linear projections
        Eigen::Matrix<Scalar, Eigen::Dynamic, Eigen::Dynamic> Q = x * W_Q_;
        Eigen::Matrix<Scalar, Eigen::Dynamic, Eigen::Dynamic> K = x * W_K_;
        Eigen::Matrix<Scalar, Eigen::Dynamic, Eigen::Dynamic> V = x * W_V_;

        // Split into heads
        std::vector<Eigen::Matrix<Scalar, Eigen::Dynamic, Eigen::Dynamic>> Q_heads, K_heads, V_heads;
        for (int h = 0; h < num_heads_; ++h) {
            Q_heads.emplace_back(Q.block(0, h * d_k_, seq_len, d_k_));
            K_heads.emplace_back(K.block(0, h * d_k_, seq_len, d_k_));
            V_heads.emplace_back(V.block(0, h * d_v_, seq_len, d_v_));
        }

        // Attention per head
        std::vector<Eigen::Matrix<Scalar, Eigen::Dynamic, Eigen::Dynamic>> head_outputs;
        for (int h = 0; h < num_heads_; ++h) {
            head_outputs.emplace_back(util::scaled_dot_product_attention(Q_heads[h], K_heads[h], V_heads[h], mask));
        }

        // Concatenate heads
        Eigen::Matrix<Scalar, Eigen::Dynamic, Eigen::Dynamic> concat(seq_len, d_model_);
        for (int h = 0; h < num_heads_; ++h) {
            concat.block(0, h * d_k_, seq_len, d_k_) = head_outputs[h];
        }

        // Final linear projection
        return concat * W_O_;
    }

private:
    int d_model_, num_heads_, d_k_, d_v_;
    Eigen::Matrix<Scalar, Eigen::Dynamic, Eigen::Dynamic> W_Q_, W_K_, W_V_, W_O_;
};

/* ------------------------------------------------------------------
 * Position‑wise Feed‑Forward Network (MLP)
 * ------------------------------------------------------------------ */
template <typename Scalar>
class FeedForward {
public:
    FeedForward(int d_model, int d_ff)
        : d_model_(d_model), d_ff_(d_ff) {
        W1_ = Eigen::Matrix<Scalar, Eigen::Dynamic, Eigen::Dynamic>(d_model, d_ff);
        b1_ = Eigen::Matrix<Scalar, Eigen::Dynamic, 1>(d_ff);
        W2_ = Eigen::Matrix<Scalar, Eigen::Dynamic, Eigen::Dynamic>(d_ff, d_model);
        b2_ = Eigen::Matrix<Scalar, Eigen::Dynamic, 1>(d_model);

        util::uniform_init(W1_);
        util::uniform_init(W2_);
        b1_.setZero();
        b2_.setZero();
    }

    Eigen::Matrix<Scalar, Eigen::Dynamic, Eigen::Dynamic>
    operator()(const Eigen::Matrix<Scalar, Eigen::Dynamic, Eigen::Dynamic>& x) {
        // x: [seq_len, d_model]
        Eigen::Matrix<Scalar, Eigen::Dynamic, Eigen::Dynamic> hidden = (x * W1_.rowwise() + b1_.transpose()).unaryExpr([](Scalar v){ return std::max(v, Scalar(0)); }); // ReLU
        return (hidden * W2_.rowwise() + b2_.transpose());
    }

private:
    int d_model_, d_ff_;
    Eigen::Matrix<Scalar, Eigen::Dynamic, Eigen::Dynamic> W1_, W2_;
    Eigen::Matrix<Scalar, Eigen::Dynamic, 1> b1_, b2_;
};

/* ------------------------------------------------------------------
 * Layer Normalization
 * ------------------------------------------------------------------ */
template <typename Scalar>
class LayerNorm {
public:
    LayerNorm(int d_model, Scalar eps = 1e-5)
        : d_model_(d_model), eps_(eps) {
        gamma_ = Eigen::Matrix<Scalar, Eigen::Dynamic, 1>(d_model);
        beta_  = Eigen::Matrix<Scalar, Eigen::Dynamic, 1>(d_model);
        gamma_.setOnes();
        beta_.setZero();
    }

    Eigen::Matrix<Scalar, Eigen::Dynamic, Eigen::Dynamic>
    operator()(const Eigen::Matrix<Scalar, Eigen::Dynamic, Eigen::Dynamic>& x) {
        // x: [seq_len, d_model]
        Eigen::VectorXd mean = x.rowwise().mean();
        Eigen::VectorXd var  = ((x.rowwise() - mean.transpose()).array().square().rowwise().mean()).matrix();

        Eigen::Matrix<Scalar, Eigen::Dynamic, Eigen::Dynamic> norm = ((x.rowwise() - mean.transpose()).array().colwise() / (var.array() + eps_).sqrt()).matrix();
        return norm.array().rowwise() * gamma_.transpose().array() + beta_.transpose().array();
    }

private:
    int d_model_;
    Scalar eps_;
    Eigen::Matrix<Scalar, Eigen::Dynamic, 1> gamma_, beta_;
};

/* ------------------------------------------------------------------
 * Transformer Encoder Block
 * ------------------------------------------------------------------ */
template <typename Scalar>
class TransformerEncoderBlock {
public:
    TransformerEncoderBlock(int d_model, int num_heads, int d_ff)
        : attn_(d_model, num_heads),
          ff_(d_model, d_ff),
          ln1_(d_model),
          ln2_(d_model) {}

    // x: [seq_len, d_model]
    Eigen::Matrix<Scalar, Eigen::Dynamic, Eigen::Dynamic>
    operator()(const Eigen::Matrix<Scalar, Eigen::Dynamic, Eigen::Dynamic>& x,
               const Eigen::Matrix<Scalar, Eigen::Dynamic, Eigen::Dynamic>* mask = nullptr) {
        // Self‑attention + residual + layer norm
        Eigen::Matrix<Scalar, Eigen::Dynamic, Eigen::Dynamic> attn_out = attn_(x, mask);
        Eigen::Matrix<Scalar, Eigen::Dynamic, Eigen::Dynamic> x1 = ln1_(x + attn_out);

        // Feed‑forward + residual + layer norm
        Eigen::Matrix<Scalar, Eigen::Dynamic, Eigen::Dynamic> ff_out = ff_(x1);
        return ln2_(x1 + ff_out);
    }

private:
    MultiHeadAttention<Scalar> attn_;
    FeedForward<Scalar> ff_;
    LayerNorm<Scalar> ln1_, ln2_;
};

/* ------------------------------------------------------------------
 * Example usage
 * ------------------------------------------------------------------ */
int main() {
    using Scalar = float;
    const int seq_len = 10;
    const int d_model = 64;
    const int num_heads = 8;
    const int d_ff = 256;

    // Dummy input: [seq_len, d_model]
    Eigen::Matrix<Scalar, Eigen::Dynamic, Eigen::Dynamic> input(seq_len, d_model);
    util::uniform_init(input, Scalar(0.5));

    // Optional mask: +inf for positions to mask (e.g. causal mask)
    Eigen::Matrix<Scalar, Eigen::Dynamic, Eigen::Dynamic> mask(seq_len, seq_len);
    mask.setZero();
    for (int i = 0; i < seq_len; ++i)
        for (int j = i + 1; j < seq_len; ++j)
            mask(i, j) = std::numeric_limits<Scalar>::infinity();

    TransformerEncoderBlock<Scalar> block(d_model, num_heads, d_ff);
    Eigen::Matrix<Scalar, Eigen::Dynamic, Eigen::Dynamic> output = block(input, &mask);

    std::cout << "Output shape: (" << output.rows() << ", " << output.cols() << ")\n";
    return 0;
}

